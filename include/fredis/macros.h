#pragma once


#define FREDIS_INHERIT_CONSTRUCTOR(cls_name, base_cls) \
    template<typename ...Args> \
    cls_name(Args&& ...args): base_cls(std::forward<Args>(args)...) {}


#define FREDIS_DECLARE_EXCEPTION(cls_name, base_cls) \
    class cls_name : public base_cls { \
     public: \
      template<typename T> \
      cls_name(const T& arg): base_cls(arg) {} \
    };

#define FREDIS_XSTR(x) #x

#define FREDIS_THROW0(cls_name) \
    throw cls_name(FREDIS_XSTR(cls_name))
