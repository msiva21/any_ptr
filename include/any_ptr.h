#pragma once
#include <utility>
#include <typeinfo>
#include <type_traits>
#include <optional>

namespace xxx {

  class bad_any_ptr_cast : public std::bad_cast
  {
  public:
    virtual const char* what() const noexcept override { return "bad any_ptr cast"; }
  };

  inline namespace ver_1 {

    /**
      The class any_ptr is type-safe container for pointers to any type 
      that, unlike std::any, allows implicit up casting.
      
      For example:

        struct Base {};
        struct Derived : public Base {};

        auto ptr = std::make_unique<Derived>();
        Derived * p = ptr.get();

        std::any a1{ p };
        std::any_cast<Derived*>( a1 ); // cast succeeds
        std::any_cast<Base*>( a1 ); // throws exceptions as there's no implicit up cast

        xxx::any_ptr a2{ p };
        xxx::any_ptr_cast<Base>( a1 ); // implicit up cast succeeds
    */
    class any_ptr
    {
    public:
      //-----------------------------------------------------
      // Canonical "Rule of 5" ctors/dtor and copy operators

      ~any_ptr() = default;

      any_ptr(any_ptr const & other) noexcept = default;

      any_ptr(any_ptr && other) noexcept = default;

      any_ptr& operator=(any_ptr const& other) noexcept = default;

      any_ptr& operator=(any_ptr && other) noexcept = default;

      //-----------------------------------------------------
      // Constructors

      // set to empty state
      any_ptr() noexcept = default;

      template<typename T>
      any_ptr(T* ptr) noexcept;

      //-----------------------------------------------------
      // Modifiers

      // swaps two any objects
      void swap(any_ptr & other) noexcept;

      // reset to empty state 
      void reset() noexcept;

      //-----------------------------------------------------
      // Observers

      // return true if not empty
      bool has_value() const noexcept { return my_throw_func != nullptr; }

      // Return type-info for held T
      const std::type_info& type() const noexcept { return (my_type_info ? *my_type_info : typeid(void)); }

    private:

      using Throw_func = void(void *);

      template<typename T>
      using HeldType = T;

      void*                   my_ptr{ nullptr };
      Throw_func *            my_throw_func{ nullptr };
      const std::type_info *  my_type_info{ nullptr };

      template <typename T>
      std::pair<T*,bool> dynamic_up_cast() const noexcept;

      template<typename T>
      static void throw_function(void * ptr)
      {
        throw static_cast<T*>(ptr);
      }

      template<typename T>
      friend std::optional<T*> any_ptr_cast(any_ptr const * any_ptr_) noexcept;

      template<typename T>
      friend T* any_ptr_cast(any_ptr const & any_ptr_);
    };

    template<typename T>
    any_ptr::any_ptr(T* ptr) noexcept
      : my_ptr{ const_cast<HeldType<T>*> (ptr) }
      , my_throw_func{ &any_ptr::throw_function<HeldType<T>> }
      , my_type_info{ & typeid(HeldType<T>*) }
    {
    }

    void any_ptr::swap(any_ptr & other) noexcept
    {
      other = std::exchange(*this, std::move(other));
    }

    // reset to empty state 
    inline void any_ptr::reset() noexcept
    {
      // Use default ctor to reset to empty state
      ::new (this) any_ptr();
    }

    template <typename T>
    std::pair<T*, bool> any_ptr::dynamic_up_cast() const noexcept
    {
      std::pair<T*, bool> result{ nullptr, false };
      if (type() == typeid(HeldType<T>*)) { // cast succeeded
        result.first = static_cast<T*>(my_ptr);
        result.second = true;
      }
      else if (has_value()) { // try an implicit up cast by throwing an exception
        try {
          my_throw_func(my_ptr);
        }
        catch (T* ptr) { // implicit up cast succeeded
          result.first = ptr;
          result.second = true;
        }
        catch (...) { // up cast failed
        }
      }
      // else cast is not permitted by C++ CV qualifier promotion rules
      return result;
    }

    //-----------------------------------------------------------------------------------------------------

    template<typename T>
    std::optional<T*> any_ptr_cast(any_ptr const * any_ptr_) noexcept
    {
      std::optional<T*> result;
      const std::pair<T*, bool> cast_result = any_ptr_->template dynamic_up_cast<T>();
      if (cast_result.second)  {
        result = cast_result.first;
      }
      return result;
    }

    template<typename T>
    T* any_ptr_cast(any_ptr const & any_ptr_)
    {
      const std::pair<T*,bool> result = any_ptr_.template dynamic_up_cast<T>();
      if (result.second) {
        return result.first;
      }
      throw bad_any_ptr_cast();
    }

  } // namespace ver_1

} // namespace xxx


namespace std {

  inline void swap(xxx::any_ptr & lhs, xxx::any_ptr & rhs)
  {
    lhs.swap(rhs);
  }

} // namespace std