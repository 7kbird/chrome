// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_LIB_ARRAY_INTERNAL_H_
#define MOJO_PUBLIC_CPP_BINDINGS_LIB_ARRAY_INTERNAL_H_

#include <new>
#include <vector>

#include "mojo/public/c/system/macros.h"
#include "mojo/public/cpp/bindings/lib/bindings_internal.h"
#include "mojo/public/cpp/bindings/lib/bindings_serialization.h"
#include "mojo/public/cpp/bindings/lib/bounds_checker.h"
#include "mojo/public/cpp/bindings/lib/buffer.h"
#include "mojo/public/cpp/bindings/lib/template_util.h"
#include "mojo/public/cpp/bindings/lib/validation_errors.h"
#include "mojo/public/cpp/environment/logging.h"

namespace mojo {
template <typename T> class Array;
class String;

namespace internal {

// std::numeric_limits<uint32_t>::max() is not a compile-time constant (until
// C++11).
const uint32_t kMaxUint32 = 0xFFFFFFFF;

std::string MakeMessageWithArrayIndex(const char* message,
                                      size_t size,
                                      size_t index);

std::string MakeMessageWithExpectedArraySize(const char* message,
                                             size_t size,
                                             size_t expected_size);

template <typename T>
struct ArrayDataTraits {
  typedef T StorageType;
  typedef T& Ref;
  typedef T const& ConstRef;

  static const uint32_t kMaxNumElements =
      (kMaxUint32 - sizeof(ArrayHeader)) / sizeof(StorageType);

  static uint32_t GetStorageSize(uint32_t num_elements) {
    MOJO_DCHECK(num_elements <= kMaxNumElements);
    return sizeof(ArrayHeader) + sizeof(StorageType) * num_elements;
  }
  static Ref ToRef(StorageType* storage, size_t offset) {
    return storage[offset];
  }
  static ConstRef ToConstRef(const StorageType* storage, size_t offset) {
    return storage[offset];
  }
};

template <typename P>
struct ArrayDataTraits<P*> {
  typedef StructPointer<P> StorageType;
  typedef P*& Ref;
  typedef P* const& ConstRef;

  static const uint32_t kMaxNumElements =
      (kMaxUint32 - sizeof(ArrayHeader)) / sizeof(StorageType);

  static uint32_t GetStorageSize(uint32_t num_elements) {
    MOJO_DCHECK(num_elements <= kMaxNumElements);
    return sizeof(ArrayHeader) + sizeof(StorageType) * num_elements;
  }
  static Ref ToRef(StorageType* storage, size_t offset) {
    return storage[offset].ptr;
  }
  static ConstRef ToConstRef(const StorageType* storage, size_t offset) {
    return storage[offset].ptr;
  }
};

template <typename T>
struct ArrayDataTraits<Array_Data<T>*> {
  typedef ArrayPointer<T> StorageType;
  typedef Array_Data<T>*& Ref;
  typedef Array_Data<T>* const& ConstRef;

  static const uint32_t kMaxNumElements =
      (kMaxUint32 - sizeof(ArrayHeader)) / sizeof(StorageType);

  static uint32_t GetStorageSize(uint32_t num_elements) {
    MOJO_DCHECK(num_elements <= kMaxNumElements);
    return sizeof(ArrayHeader) + sizeof(StorageType) * num_elements;
  }
  static Ref ToRef(StorageType* storage, size_t offset) {
    return storage[offset].ptr;
  }
  static ConstRef ToConstRef(const StorageType* storage, size_t offset) {
    return storage[offset].ptr;
  }
};

// Specialization of Arrays for bools, optimized for space. It has the
// following differences from a generalized Array:
// * Each element takes up a single bit of memory.
// * Accessing a non-const single element uses a helper class |BitRef|, which
// emulates a reference to a bool.
template <>
struct ArrayDataTraits<bool> {
  // Helper class to emulate a reference to a bool, used for direct element
  // access.
  class BitRef {
   public:
    ~BitRef();
    BitRef& operator=(bool value);
    BitRef& operator=(const BitRef& value);
    operator bool() const;
   private:
    friend struct ArrayDataTraits<bool>;
    BitRef(uint8_t* storage, uint8_t mask);
    BitRef();
    uint8_t* storage_;
    uint8_t mask_;
  };

  // Because each element consumes only 1/8 byte.
  static const uint32_t kMaxNumElements = kMaxUint32;

  typedef uint8_t StorageType;
  typedef BitRef Ref;
  typedef bool ConstRef;

  static uint32_t GetStorageSize(uint32_t num_elements) {
    return sizeof(ArrayHeader) + ((num_elements + 7) / 8);
  }
  static BitRef ToRef(StorageType* storage, size_t offset) {
    return BitRef(&storage[offset / 8], 1 << (offset % 8));
  }
  static bool ToConstRef(const StorageType* storage, size_t offset) {
    return (storage[offset / 8] & (1 << (offset % 8))) != 0;
  }
};

// Array type information needed for valdiation.
template <uint32_t in_expected_num_elements,
          bool in_element_is_nullable,
          typename InElementValidateParams>
class ArrayValidateParams {
 public:
  // Validation information for elements. It is either another specialization of
  // ArrayValidateParams (if elements are arrays) or NoValidateParams.
  typedef InElementValidateParams ElementValidateParams;

  // If |expected_num_elements| is not 0, the array is expected to have exactly
  // that number of elements.
  static const uint32_t expected_num_elements = in_expected_num_elements;
  // Whether the elements are nullable.
  static const bool element_is_nullable = in_element_is_nullable;
};

// NoValidateParams is used to indicate the end of an ArrayValidateParams chain.
class NoValidateParams {
};

// What follows is code to support the serialization of Array_Data<T>. There
// are two interesting cases: arrays of primitives and arrays of objects.
// Arrays of objects are represented as arrays of pointers to objects.

template <typename T, bool is_handle> struct ArraySerializationHelper;

template <typename T>
struct ArraySerializationHelper<T, false> {
  typedef typename ArrayDataTraits<T>::StorageType ElementType;

  static void EncodePointersAndHandles(const ArrayHeader* header,
                                       ElementType* elements,
                                       std::vector<Handle>* handles) {
  }

  static void DecodePointersAndHandles(const ArrayHeader* header,
                                       ElementType* elements,
                                       std::vector<Handle>* handles) {
  }

  template <bool element_is_nullable, typename ElementValidateParams>
  static bool ValidateElements(const ArrayHeader* header,
                               const ElementType* elements,
                               BoundsChecker* bounds_checker) {
    MOJO_COMPILE_ASSERT(!element_is_nullable,
                        Primitive_type_should_be_non_nullable);
    MOJO_COMPILE_ASSERT(
        (IsSame<ElementValidateParams, NoValidateParams>::value),
        Primitive_type_should_not_have_array_validate_params);
    return true;
  }
};

template <>
struct ArraySerializationHelper<Handle, true> {
  typedef ArrayDataTraits<Handle>::StorageType ElementType;

  static void EncodePointersAndHandles(const ArrayHeader* header,
                                       ElementType* elements,
                                       std::vector<Handle>* handles);

  static void DecodePointersAndHandles(const ArrayHeader* header,
                                       ElementType* elements,
                                       std::vector<Handle>* handles);

  template <bool element_is_nullable, typename ElementValidateParams>
  static bool ValidateElements(const ArrayHeader* header,
                               const ElementType* elements,
                               BoundsChecker* bounds_checker) {
    MOJO_COMPILE_ASSERT(
        (IsSame<ElementValidateParams, NoValidateParams>::value),
        Handle_type_should_not_have_array_validate_params);

    for (uint32_t i = 0; i < header->num_elements; ++i) {
      if (!element_is_nullable &&
          elements[i].value() == kEncodedInvalidHandleValue) {
        ReportValidationError(VALIDATION_ERROR_UNEXPECTED_INVALID_HANDLE);
        return false;
      }
      if (!bounds_checker->ClaimHandle(elements[i])) {
        ReportValidationError(VALIDATION_ERROR_ILLEGAL_HANDLE);
        return false;
      }
    }
    return true;
  }
};

template <typename H>
struct ArraySerializationHelper<H, true> {
  typedef typename ArrayDataTraits<H>::StorageType ElementType;

  static void EncodePointersAndHandles(const ArrayHeader* header,
                                       ElementType* elements,
                                       std::vector<Handle>* handles) {
    ArraySerializationHelper<Handle, true>::EncodePointersAndHandles(
        header, elements, handles);
  }

  static void DecodePointersAndHandles(const ArrayHeader* header,
                                       ElementType* elements,
                                       std::vector<Handle>* handles) {
    ArraySerializationHelper<Handle, true>::DecodePointersAndHandles(
        header, elements, handles);
  }

  template <bool element_is_nullable, typename ElementValidateParams>
  static bool ValidateElements(const ArrayHeader* header,
                               const ElementType* elements,
                               BoundsChecker* bounds_checker) {
    return ArraySerializationHelper<Handle, true>::
        ValidateElements<element_is_nullable, ElementValidateParams>(
            header, elements, bounds_checker);
  }
};

template <typename P>
struct ArraySerializationHelper<P*, false> {
  typedef typename ArrayDataTraits<P*>::StorageType ElementType;

  static void EncodePointersAndHandles(const ArrayHeader* header,
                                       ElementType* elements,
                                       std::vector<Handle>* handles) {
    for (uint32_t i = 0; i < header->num_elements; ++i)
      Encode(&elements[i], handles);
  }

  static void DecodePointersAndHandles(const ArrayHeader* header,
                                       ElementType* elements,
                                       std::vector<Handle>* handles) {
    for (uint32_t i = 0; i < header->num_elements; ++i)
      Decode(&elements[i], handles);
  }

  template <bool element_is_nullable, typename ElementValidateParams>
  static bool ValidateElements(const ArrayHeader* header,
                               const ElementType* elements,
                               BoundsChecker* bounds_checker) {
    for (uint32_t i = 0; i < header->num_elements; ++i) {
      if (!element_is_nullable && !elements[i].offset) {
        ReportValidationError(VALIDATION_ERROR_UNEXPECTED_NULL_POINTER);
        return false;
      }
      if (!ValidateEncodedPointer(&elements[i].offset)) {
        ReportValidationError(VALIDATION_ERROR_ILLEGAL_POINTER);
        return false;
      }
      if (!ValidateCaller<P, ElementValidateParams>::Run(
              DecodePointerRaw(&elements[i].offset), bounds_checker)) {
        return false;
      }
    }
    return true;
  }

 private:
  template <typename T, typename Params>
  struct ValidateCaller {
    static bool Run(const void* data, BoundsChecker* bounds_checker) {
      MOJO_COMPILE_ASSERT(
          (IsSame<Params, NoValidateParams>::value),
          Struct_type_should_not_have_array_validate_params);

      return T::Validate(data, bounds_checker);
    }
  };

  template <typename T, typename Params>
  struct ValidateCaller<Array_Data<T>, Params> {
    static bool Run(const void* data, BoundsChecker* bounds_checker) {
      return Array_Data<T>::template Validate<Params>(data, bounds_checker);
    }
  };
};

template <typename T>
class Array_Data {
 public:
  typedef ArrayDataTraits<T> Traits;
  typedef typename Traits::StorageType StorageType;
  typedef typename Traits::Ref Ref;
  typedef typename Traits::ConstRef ConstRef;
  typedef ArraySerializationHelper<T, IsHandle<T>::value> Helper;

  // Returns NULL if |num_elements| or the corresponding storage size cannot be
  // stored in uint32_t.
  static Array_Data<T>* New(size_t num_elements, Buffer* buf) {
    if (num_elements > Traits::kMaxNumElements)
      return NULL;

    uint32_t num_bytes =
        Traits::GetStorageSize(static_cast<uint32_t>(num_elements));
    return new (buf->Allocate(num_bytes)) Array_Data<T>(
        num_bytes, static_cast<uint32_t>(num_elements));
  }

  template <typename Params>
  static bool Validate(const void* data, BoundsChecker* bounds_checker) {
    if (!data)
      return true;
    if (!IsAligned(data)) {
      ReportValidationError(VALIDATION_ERROR_MISALIGNED_OBJECT);
      return false;
    }
    if (!bounds_checker->IsValidRange(data, sizeof(ArrayHeader))) {
      ReportValidationError(VALIDATION_ERROR_ILLEGAL_MEMORY_RANGE);
      return false;
    }
    const ArrayHeader* header = static_cast<const ArrayHeader*>(data);
    if (header->num_elements > Traits::kMaxNumElements ||
        header->num_bytes < Traits::GetStorageSize(header->num_elements)) {
      ReportValidationError(VALIDATION_ERROR_UNEXPECTED_ARRAY_HEADER);
      return false;
    }
    if (Params::expected_num_elements != 0 &&
        header->num_elements != Params::expected_num_elements) {
      ReportValidationError(VALIDATION_ERROR_UNEXPECTED_ARRAY_HEADER);
      return false;
    }
    if (!bounds_checker->ClaimMemory(data, header->num_bytes)) {
      ReportValidationError(VALIDATION_ERROR_ILLEGAL_MEMORY_RANGE);
      return false;
    }

    const Array_Data<T>* object = static_cast<const Array_Data<T>*>(data);
    return Helper::template ValidateElements<
        Params::element_is_nullable, typename Params::ElementValidateParams>(
            &object->header_, object->storage(), bounds_checker);
  }

  size_t size() const { return header_.num_elements; }

  Ref at(size_t offset) {
    MOJO_DCHECK(offset < static_cast<size_t>(header_.num_elements));
    return Traits::ToRef(storage(), offset);
  }

  ConstRef at(size_t offset) const {
    MOJO_DCHECK(offset < static_cast<size_t>(header_.num_elements));
    return Traits::ToConstRef(storage(), offset);
  }

  StorageType* storage() {
    return reinterpret_cast<StorageType*>(
        reinterpret_cast<char*>(this) + sizeof(*this));
  }

  const StorageType* storage() const {
    return reinterpret_cast<const StorageType*>(
        reinterpret_cast<const char*>(this) + sizeof(*this));
  }

  void EncodePointersAndHandles(std::vector<Handle>* handles) {
    Helper::EncodePointersAndHandles(&header_, storage(), handles);
  }

  void DecodePointersAndHandles(std::vector<Handle>* handles) {
    Helper::DecodePointersAndHandles(&header_, storage(), handles);
  }

 private:
  Array_Data(uint32_t num_bytes, uint32_t num_elements) {
    header_.num_bytes = num_bytes;
    header_.num_elements = num_elements;
  }
  ~Array_Data() {}

  internal::ArrayHeader header_;

  // Elements of type internal::ArrayDataTraits<T>::StorageType follow.
};
MOJO_COMPILE_ASSERT(sizeof(Array_Data<char>) == 8, bad_sizeof_Array_Data);

// UTF-8 encoded
typedef Array_Data<char> String_Data;

template <typename T, bool kIsMoveOnlyType> struct ArrayTraits {};

template <typename T> struct ArrayTraits<T, false> {
  typedef T StorageType;
  typedef typename std::vector<T>::reference RefType;
  typedef typename std::vector<T>::const_reference ConstRefType;
  typedef ConstRefType ForwardType;
  static inline void Initialize(std::vector<T>* vec) {
  }
  static inline void Finalize(std::vector<T>* vec) {
  }
  static inline ConstRefType at(const std::vector<T>* vec, size_t offset) {
    return vec->at(offset);
  }
  static inline RefType at(std::vector<T>* vec, size_t offset) {
    return vec->at(offset);
  }
  static inline void Resize(std::vector<T>* vec, size_t size) {
    vec->resize(size);
  }
  static inline void PushBack(std::vector<T>* vec, ForwardType value) {
    vec->push_back(value);
  }
};

template <typename T> struct ArrayTraits<T, true> {
  struct StorageType {
    char buf[sizeof(T) + (8 - (sizeof(T) % 8)) % 8];  // Make 8-byte aligned.
  };
  typedef T& RefType;
  typedef const T& ConstRefType;
  typedef T ForwardType;
  static inline void Initialize(std::vector<StorageType>* vec) {
    for (size_t i = 0; i < vec->size(); ++i)
      new (vec->at(i).buf) T();
  }
  static inline void Finalize(std::vector<StorageType>* vec) {
    for (size_t i = 0; i < vec->size(); ++i)
      reinterpret_cast<T*>(vec->at(i).buf)->~T();
  }
  static inline ConstRefType at(const std::vector<StorageType>* vec,
                                size_t offset) {
    return *reinterpret_cast<const T*>(vec->at(offset).buf);
  }
  static inline RefType at(std::vector<StorageType>* vec, size_t offset) {
    return *reinterpret_cast<T*>(vec->at(offset).buf);
  }
  static inline void Resize(std::vector<StorageType>* vec, size_t size) {
    size_t old_size = vec->size();
    for (size_t i = size; i < old_size; i++)
      reinterpret_cast<T*>(vec->at(i).buf)->~T();
    ResizeStorage(vec, size);
    for (size_t i = old_size; i < vec->size(); i++)
      new (vec->at(i).buf) T();
  }
  static inline void PushBack(std::vector<StorageType>* vec, RefType value) {
    size_t old_size = vec->size();
    ResizeStorage(vec, old_size + 1);
    new (vec->at(old_size).buf) T(value.Pass());
  }
  static inline void ResizeStorage(std::vector<StorageType>* vec, size_t size) {
    if (size <= vec->capacity()) {
      vec->resize(size);
      return;
    }
    std::vector<StorageType> new_storage(size);
    for (size_t i = 0; i < vec->size(); i++)
      new (new_storage.at(i).buf) T(at(vec, i).Pass());
    vec->swap(new_storage);
    Finalize(&new_storage);
  }
};

template <> struct WrapperTraits<String, false> {
  typedef String_Data* DataType;
};

}  // namespace internal
}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_LIB_ARRAY_INTERNAL_H_
