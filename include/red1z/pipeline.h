// -*- C++ -*-
#ifndef RED1Z_PIPELINE_H
#define RED1Z_PIPELINE_H

#include "red1z/basic_pipeline.h"

namespace red1z {
  template <class T>
  class Pipeline :
    public impl::BasicPipeline<T, Pipeline<T>>
  {
    using Base =  impl::BasicPipeline<T, Pipeline<T>>;
    friend Base;
  public:
    using Base::Base;
  private:
    template <class Resolver>
    void _execute(int n, Resolver& resolve) {
      for (int i = 0; i < n; ++i) {
        resolve(this->m_queue.get_reply());
      }
    }
  };

}

#endif
