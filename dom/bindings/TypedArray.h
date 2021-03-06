/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_TypedArray_h
#define mozilla_dom_TypedArray_h

#include "jsfriendapi.h"

namespace mozilla {
namespace dom {

/*
 * Various typed array classes for argument conversion.  We have a base class
 * that has a way of initializing a TypedArray from an existing typed array, and
 * a subclass of the base class that supports creation of a relevant typed array
 * or array buffer object.
 */
template<typename T, typename U,
         U* GetData(JSObject*, JSContext*),
         uint32_t GetLength(JSObject*, JSContext*)>
struct TypedArray_base {
  TypedArray_base(JSContext* cx, JSObject* obj) :
    mData(static_cast<T*>(GetData(obj, cx))),
    mLength(GetLength(obj, cx)),
    mObj(obj)
  {}

  T* const mData;
  const uint32_t mLength;
  JSObject* const mObj;
};


template<typename T, typename U,
         U* GetData(JSObject*, JSContext*),
         uint32_t GetLength(JSObject*, JSContext*),
         JSObject* CreateNew(JSContext*, uint32_t)>
struct TypedArray : public TypedArray_base<T,U,GetData,GetLength> {
  TypedArray(JSContext* cx, JSObject* obj) :
    TypedArray_base<T,U,GetData,GetLength>(cx, obj)
  {}
  
  static inline JSObject*
  Create(JSContext* cx, uint32_t length, T* data = NULL) {
    JSObject* obj = CreateNew(cx, length);
    if (!obj) {
      return NULL;
    }
    if (data) {
      T* buf = static_cast<T*>(GetData(obj, cx));
      memcpy(buf, data, length*sizeof(T));
    }
    return obj;
  }
};

typedef TypedArray<int8_t, int8_t, JS_GetInt8ArrayData, JS_GetTypedArrayLength,
                   JS_NewInt8Array>
        Int8Array;
typedef TypedArray<uint8_t, uint8_t, JS_GetUint8ArrayData,
                   JS_GetTypedArrayLength, JS_NewUint8Array>
        Uint8Array;
typedef TypedArray<uint8_t, uint8_t, JS_GetUint8ClampedArrayData,
                   JS_GetTypedArrayLength, JS_NewUint8Array>
        Uint8ClampedArray;
typedef TypedArray<int16_t, int16_t, JS_GetInt16ArrayData,
                   JS_GetTypedArrayLength, JS_NewInt16Array>
        Int16Array;
typedef TypedArray<uint16_t, uint16_t, JS_GetUint16ArrayData,
                   JS_GetTypedArrayLength, JS_NewUint16Array>
        Uint16Array;
typedef TypedArray<int32_t, int32_t, JS_GetInt32ArrayData,
                   JS_GetTypedArrayLength, JS_NewInt32Array>
        Int32Array;
typedef TypedArray<uint32_t, uint32_t, JS_GetUint32ArrayData,
                   JS_GetTypedArrayLength, JS_NewUint32Array>
        Uint32Array;
typedef TypedArray<float, float, JS_GetFloat32ArrayData, JS_GetTypedArrayLength,
                   JS_NewFloat32Array>
        Float32Array;
typedef TypedArray<double, double, JS_GetFloat64ArrayData,
                   JS_GetTypedArrayLength, JS_NewFloat64Array>
        Float64Array;
typedef TypedArray_base<uint8_t, void, JS_GetArrayBufferViewData,
                        JS_GetArrayBufferViewByteLength>
        ArrayBufferView;
typedef TypedArray<uint8_t, uint8_t, JS_GetArrayBufferData,
                   JS_GetArrayBufferByteLength, JS_NewArrayBuffer>
        ArrayBuffer;

} // namespace dom
} // namespace mozilla

#endif /* mozilla_dom_TypedArray_h */
