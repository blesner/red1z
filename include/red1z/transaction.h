// -*- C++ -*-
#ifndef RED1Z_TRANSACTION_H
#define RED1Z_TRANSACTION_H

#include "red1z/basic_pipeline.h"

namespace red1z {
  template <class T>
  class Transaction :
    public impl::BasicPipeline<T, Transaction<T>>
  {
    using Base =  impl::BasicPipeline<T, Transaction<T>>;
    friend Base;
  public:
    Transaction(impl::Context& c) : Base(c) {
      this->m_queue.append("*1\r\n$5\r\nMULTI\r\n");
    }

  private:
    template <class Resolver>
    void _execute(int n, Resolver& resolve) {
      this->m_queue.append("*1\r\n$4\r\nEXEC\r\n");
      //discard MULTI + all the commands
      this->m_queue.discard(n + 1);

      auto elements = this->m_queue.get_reply().array();//consume EXEC

      if (n != static_cast<int>(elements.size())) {
        throw Error("unexpected replies in transaction");
      }

      for (auto& reply : elements) {
        resolve(std::move(reply));
      }
    }

    void _discard() {
      this->m_queue.append("*1\r\n$7\r\nDISCARD\r\n");
    }
  };
}

#endif //RED1Z_TRANSACTION_H
