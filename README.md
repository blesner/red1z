# red1z
A c++17 REDIS client.

## About

The name comes from the `c++1z` used by compilers before they had full C++17 support.
I know there are many Redis clients out there, but I just wanted to write my own. First to get used to some of C++17 features and then (and  mostly) as an API design experiment.


This project has *no* external dependencies. And so far is supported on Linux with, of course, a C++17 compiler.

# Features
* Modern, Type-Safe and intuitive API.
* Interpret raw redis data with C++ types on top
  * e.g. you can store an `int` and read an `int`, only using 4 bytes in Redis
  * Custom types I/O support
* Low overhead: avoid copying data around, uses POSIX `writev()` for pipelining.

## Command Support
Most of REDIS 5 commands are supported expect for some informational commands, `Server`, `Geo`, `Scripting` commands are also missing. There is some basic support of `Pub/Sub` but I've yet to come with a satisfying API design.

## Thread Safety
IMHO **every** project should cleary state their thread-safety properties, so here you are:

An instance of  `red1z::Redis` cannot be used between different threads. Use one instance per thread and you'll be OK ;)


# Quickstart

## Building & Installing
`red1z` uses CMake, juist create a `build` directory run `cmake` from it then run `make` and `make install`.
Link using `-lred1z`.

## Error Reporting
`red1z` uses exceptions, and all derive from `red1z::Error` which in turns derives from `std::runtime_error`.

## Usage

The entrypoint of `red1z` is the `red1z::Redis` class, each (except for transactions) redis command has a corresponding method (lowercased) on `red1z::Redis`

You can specify the result type for every command (where relevant) as a template parameter `T` or just use the default of `std::string`.
If the command may return no value (like `get<T>`) the returned type is `std::optional<T>`.

The idea is to be able to use any type (under conditions, discussed later) as Redis values **and** keys, and have a strongly typed C++ API on top. In a nutshell, every value returned as a `SimpleString` or `BulkString`, can be interpreted with a type.

Commands come in two forms: the *direct* form and the *bound* form:
### Direct form
In the *direct* form the commands returns directy its value. The direct form accepts an optional type paremeter for the returned value (possibly wrapped in `std::optional`)
```c++
auto r = red1z::Redis::from_url("redis://:password@hostname");
auto v = r.get("my key"); //v is std::optional<std::string>
auto i = r.get<int>("key"); //i is std::optional<int>

auto len = r.llen("a-list"); //len is std::int64_t, no std::optional here
```

### Bound form
In the *bound* form the command is given an output parameter. The ouput paremater is either a pointer to the output variable
*or* an output iterator. For commands that return a dynamic number of values the output parameter _*must*_ be an output iterator. The general syntax is:
`r[OUTPUT-PARAM].command(args...);`

Commands that return an `std::optional<T>` in direct form accepts a `T*` in bound form and return a `bool` indicating wether the output has been written to. In bound form the output type paremeter is deduced from the output parameter. Its still possible to pass an output-type parameter `T`, in this case the stored data will be read from Redis as a `T` instance and then converted to the output paremeter's type. This allows to apply a conversion (for exemple between numeric types, see below).

```c++
//POINTER VARIANT
std::string v;
if (r[&v].get("my key")) {
  //a value as been read into v
}
else {
  //no value read, v is unchanged
}

r.mset("key1", 10, "key2", 20); //store 2 ints in key1 and key2
double d;
r[&d].get("key1"); // will throw: the value stored at key1 is too small for a double.
r[&d].get<int>("key1"); //OK: read the data as an int then ASSIGN it to d


//OUTPUT ITERATOR VARIANT
r.lpush("somelist", 11, 12, 13); //store3 ints into a list

std::vector<int> vi;
auto const n = r[std::back_inserter(vi)].lrange("somelist", 0, -1);
//n is the number of elements inserted in vi (here 3)

std::vector<double> vd;
//read elements as ints and store them as double into vd
r[std::back_inserter(vec)].lrange<int>("somelist", 0, -1);

```



### Quick Example
```c++
#include "red1z/red1z.h"

int main(int, char**) {
  auto r = red1z::Redis::from_url("redis://:password@hostname");
  r.set("my_key", "value");
  if (auto v = r.get("my_key")) {
    // here v is std::optional<std::string> (the default value type is std::string in red1z)
    std::cout << "got value: " << *v << '\n';
  }

  r.set("number", 42); //where the BINARY value of the int 42 will be stored;
  if (auto v = r.get<int>("number")) {
    //*v is an int with value 42
  }

  //variadic commands are supported
  r.mset("key1", 10, "key2", 20, "key3", "foo");

  //when you don't kown in advance the number of arguments use unpack():
  std::vector<std::tuple<std::string, int>> kv = {{"key1", 10}, {key2, 20}};
  r.mset("key3", "bar", red1z::unpack(kv), "you can mix arguments", "with uses of unpack()");
  //unpack() takes any container whose value type T has the 'tuple protocol'
  //with std::tuple_size_v<T> == 2; i.e. std::tuple<K, V>, std::array<T, 2>, std::pair<K, V>,
  //or anything that has the proper specializations for std::get and std::tuple_size
  //the is also an overload of unpack() taking a interator pair

  if (auto v = r.get<std::array<int,1>>("number")) {
    //also works out-of-the box since an std::array of 1 int has the same binary representation as one int alone
    //(*v)[0] is an int with value 42
  }
  if (auto v = r.get<std::array<short, 2>>("number")) {
    //also works, the 4 bytes of your in will just be split into 2 shorts
  }
  if (auto v = r.get<std::vector<int>>("number")) {
    //when using get<std::vector<T>> the result with be a vector with as many T's as possible
    //(if there is some data left in the value, or not enough data this is an error)
  }
  //output parameters are sypported with a different syntax:
  int num;
  r[&num].get("number"); //no need to specify the output type, it is automatically deduced
  //you can also use output iterators:
  std::list<int> lst;
  if (r[std::front_inserter(lst)].get("number")) {
    //a value has been inserted
  }
  else {
    //no value returned form get()
  }

  //generally valriadic commands return tuples : here std::tuple<std::optioanl<int>, std::optional<std::string>>
  auto const [n, str] = r.mget<int, std::string>("key1", "key3");

  //some arguments may change the return type, e.g. when using unpack
  //we don't know in advance the number of returned values:
  std::vector<std::string> args = {"key1", "key2"};
  std::vector<std::optional<int>> res = r.mget<int>(red1z::unpack(args));
  //the (optional) type parameter must be valid for all values returned (the default of std::string always works).


}
```

## Passing Flags
The flags are explicit and do not introduce new methods they are juste passed as arguments to redis commands. The flags are functions living in the namespace `red1z::flags`. The returned type may differ depending on the passed flags.

```c++
namespace flg = rd1z::flags;
r.set("key", 42, flg::ex(10)); //set the EX (expire) flag in 10 seconds

r.zrange("sorted-set", 0, -1) //returns a std::vector<std::string>
r.zrange<T>("sorted-set", 0, -1) //returns a std::vector<T>

//BUT flags may change the return type

r.zrange<T>("sorted-set", 0, -1, flg::withscores()); //returns a std::vector<std::tuple<T, double>>

std::map<std::string, double> m;
r[std::inserter(m, m.end())].zrange("sorted-set", 0, -1, flg::withscores());

std::set<std::string> s;
r[std::inserter(s, s.end())].zrange("sorted-set", 0, -1);
```

## Transactions
There are two forms of transactions in `red1z`, *static* and *dynamic* transactions, the differ in the way return arguments are typed. Note that transactions use pipelined transfers, nothing is sent to the server until `execute()` is called.

### Static transaction
When you know exactly at compile-time the commands you want in a transaction the *static* form is preferred, it provides better typing and a minimal ammount of runtime penalty. The transaction is made using the variadic overload of `red1z::Redis::transaction(...)`. The returned value is a `std::tuple` of the returned type of each command.

```c++
auto& cmd = red1z::commands; //red1z::command is an object with all redis commands as methods but do not execute anyting.
auto r = red1z::Redis::from_url("redis://:password@hostname");

//STATIC transaction: notice the use of methods on 'cmd' as arguments:
int v;
auto const [r1, r2, r3] = r.transaction(cmd.get<int>("key1"), cmd.set("K", "bar"), cmd[&v].get("key2"));
//r1 is std::optional<int>, r2 is bool, r3 is bool
```

### Dynamic transaction
When you don't know in advance the number of commands in your transaction use the static form of `red1z::Redis::transaction()` which return a *transaction object* with all the redis commands as methods.
The `transaction<T>()` method takes an optional type parameter `T` defaulted to `std::any` to hold the results of the individual commands. The free functions `red1z::opt<T>(std::any const&)` and `red1z::get<T>(std::any const&)` are provided to easily access the value held in an `std::any` instance, by performing `std::any_cast<std::optional<T>>()` and `std::any_cast<T>()`, respectively. If the result of all the commands in the transaction can be held in a single type `T` you can use an explict type argument here.

The transaction must be explicitely executed by calling `execute()` which returns an `std::vector<T>`,
or conversely it can be cancelled by calling `discard()`.

```c++
auto tr = r.transaction();
tr.get<int>("key1");
tr.set("K", "bar");
int v;
tr[&v].get("key2"); //bound form is allowed ;)

auto const res = tr.execute(); //res is std::vector<std::any>
if (auto i = red1z::opt<int>(r[0])) {
  //i is std::optional<int>
  //use *i
}
else {
  //no value at "key1"
}

if (red1z::get<bool>(r[1])) {
  // the set() command was successful
}

if (red1z::get<bool>(r[2])) {
  //v has a value
}
else {
  //v has no value
}
```
## Pipelines
Pipelines behave very similarily as transactions do, both in staic and dynamic variants. Just use `red1z::Redis::pipeline()` instead of `transaction()`. The only difference is that pipelines have no `discard()` method.

# Custom types I/O
The goal of `red1z` is to offer typing on `SimpleString` and `BulkString` values *and keys*.
The fundamental types types (`int`, `float`, ...) has native support in `red1z`, `std::string`, the default value type, is obviously also supported. Moreover, any type `T` satisfying `std::is_trivially_copyable_v<T>` **and** `std::is_standard_layout_v<T>` works out of the box, as well as containers like `std::tuple`, `std::array`, `std::vector`, `std::list`, etc.  of such types. When using a container the raw value size must be a mutiple of the size of the value_type size, otherwise an exception will be thrown at runtime.
A simple `struct` might likely work without any further customization.

For those user-defined types that do not meet the requirements for automatic serialization, you can define your own serialization code by specializing the template `red1z::io<T>`:
```c++

//the type whe whant to support
//i.e. I want to write:
// CustomType x;
// r.set("KEY", x);
// r.get<CustomType>("KEY");
class CustomType {
  //...
  CustomType(...) {
    //...
  }
};

//here is is
namespace red1z {
  template <>
  struct io<CustomType> {
    static std::string view(CustomType const& x) {
      //seralize your type into a std::string
      //if you're bold enough you can instead  return a std::string_view mapping some internal data of CustomType
      //just make sure that you instance lifetime extends after the command, or the execute() call
      //in the case of a transaction or pipeline otherwise things will get very, very nasty.
    }

    static CustomType read(std::string_view data) {
      //deserialize your object here, red1z assumes that all the contents of data will be consumed
    }
  };
}

//Enjoy !
```
Well that's nice, we can use *single* instances of `CutomType` but still none of those will work:
```c++
r.get<std::tuple<CustomType, std::string>>("some_key");
r.get<std::vector<CustomType>>("some-other-key");
```
This is because `red1z` must know, given some amount of bytes retruend by REDIS, how much of them should go to `CustomType` and to `std::string`. The bad news is that if your seralized data size (returned by `io<CustomType>::view()`) varies depending on the object to serialize you're done. But here's the good news: if the serialization always has the same size `N` you can tell that to `red1z`, just by inheriting your `io<CustomType>` specialisation from `item_io<N>` :

```c++
namespace red1z {
  template <>
  struct io<CustomType> : item_io<N> {
    //same as before !
  };
}
```
