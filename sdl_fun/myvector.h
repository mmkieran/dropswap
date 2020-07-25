#pragma once


#include <stdio.h>
#include <stdlib.h>
#include "myvector.h"
#include <math.h>


template <typename T>
struct Vector {

   double growth;
   int length;
   T* data; //Pointer to memory allocated for data array
   int capacity;

};

template <typename T>
Vector<T>* vectorCreate(int capacity, double growth) {
   Vector<T>* vector = (Vector<T>*)malloc(sizeof(Vector<T>));
   T* data = nullptr;
   if (capacity <= 0) { capacity = 10; }
   if (growth <= 1) { growth = 2; }
   if (vector) {
      data = (T*)malloc(sizeof(T) * capacity);
      if (data) {
         vector->data = data;
         vector->growth = growth;
         vector->capacity = capacity;
         vector->length = 0;

         return vector;
      }
   }
   return nullptr;
}

template <typename T>
int vectorDestroy(Vector<T>* vector) {
   if (vector) {
      if (vector->data) { free(vector->data); }
      free(vector);
      return 1;
   }
   return 0;
}

template <typename T>
bool vectorIsEmpty(Vector<T>* vector) {
   if (vector) {
      if (vector->length > 0) { return false; }
   }
   return true;
}

template <typename T>
int vectorSize(Vector<T>* vector) {
   if (vector) {
      return vector->length;
   }
   return 0;
}

template <typename T>
int vectorMaxSize(Vector<T>* vector) {
   if (vector) {
      return vector->capacity;
   }
   return 0;
}

template <typename T>
static int _calcBuffer(Vector<T>* vector) {
   int out;
   out = (double)vector->length * vector->growth + 1;
   if (out <= 0) { return 10; }
   return out;
}

template <typename T>
static int _vectorReserve(Vector<T>* vector, int newSize) {
   if (vector) {
      T* resizedData = (T*)realloc(vector->data, sizeof(T) * newSize);
      if (resizedData) {
         //fprintf(stdout, "Resizing vector: %d to %d\n", vector->capacity, newSize);
         vector->data = resizedData;
         vector->capacity = newSize;
         return 1;
      }
   }
   return 0;
}

template <typename T>
int vectorReserve(Vector<T>* vector, int newSize) {
   if (newSize <= vector->length) { return 0; }
   if (vector) {
      int check = _vectorReserve(vector, newSize);
      if (check != 0) { return 1; }
   }
   return 0;
}

template <typename T>
static int _vectorSetOutside(Vector<T>* vector, int index, T value) {
   if (vector) {
      if (index > 0 && index <= vector->capacity) {
         vector->data[index - 1] = value;
         return 1;
      }
   }
   return 0;
}

template <typename T>
int vectorResize(Vector<T>* vector, int newSize, int val) {
   if (newSize <= 0) { return 0; }
   if (vector) {
      if (newSize > vector->length && newSize > vector->capacity) {
         int check = _vectorReserve(vector, newSize);
         if (check == 0) { return 0; }
         for (int i = vector->length + 1; i <= newSize; i++) {
            _vectorSetOutside(vector, i, val);
         }
         vector->length = newSize;
         return 1;
      }

      if (newSize < vector->length) {
         int check = _vectorReserve(vector, newSize);
         if (check == 0) { return 0; }
         vector->length = newSize;
         return 1;
      }
   }
   return 0;
}

template <typename T>
int vectorShrink(Vector<T>* vector) {
   int out = _vectorReserve(vector, vector->length);
   if (out == 0) { return 0; }
   return 1;
}

template <typename T>
static T* _vectorGet(Vector<T>* vector, int index) {
   T* out;
   if (vector) {
      if (index > 0 && index <= vector->length) {
         out = (vector->data + index - 1);
         return out;
      }
   }
   return nullptr;
}

template <typename T>
T* vectorGet(Vector<T>* vector, int index) {
   T* out = nullptr;
   out = _vectorGet(vector, index);
   if (!out) { return nullptr; }
   return out;
}

template <typename T>
int vectorSet(Vector<T>* vector, int index, T value) {
   if (vector) {
      if (index > 0 && index <= vector->length) {
         vector->data[index - 1] = value;
         return 1;
      }
   }
   return 0;
}

template <typename T>
int vectorPushBack(Vector<T>* vector, T value) {
   int check = 0;
   if (vector) {
      if (vector->length == vector->capacity) {
         check = vectorReserve(vector, _calcBuffer(vector));
         if (check == 0) { return 0; }
      }
      if (vector->data) {
         check = _vectorSetOutside(vector, vector->length + 1, value);
         if (check == 0) { return 0; }
         vector->length += 1;
         return 1;
      }
   }
   return 0;
}

template <typename T>
T vectorPopBack(Vector<T>* vector) {
   T out;
   if (vector) {
      if (vector->length > 0) {
         T* end = _vectorGet(vector, vector->length);
         if (end) {
            out = *end;
            vector->length -= 1;
            return out;
         }
      }
   }
   return NULL;
}

template <typename T>
T* vectorFront(Vector<T>* vector) {
   T* out = _vectorGet(vector, 1);
   if (out) { return out; }
   return nullptr;
}

template <typename T>
T* vectorBack(Vector<T>* vector) {
   T* out = _vectorGet(vector, vector->length);
   if (out) { return out; }
   return nullptr;
}

template <typename T>
int vectorSwap(Vector<T>* vector, int first, int second) {
   if (first < 0 && first > vector->length) {
      return 0;
   }
   if (second <= 0 && second > vector->length) {
      return 0;
   }
   T* t1 = _vectorGet(vector, first);
   T* t2 = _vectorGet(vector, second);
   if (t1 && t2) {
      T temp = *t1;
      *t1 = *t2;
      *t2 = temp;
      return 1;
   }
   return 0;
}

template <typename T>
int vectorInsert(Vector<T>* vector, int index, T value) {
   if (index < 0 || index > vector->length) {
      return 0;
   }

   if (vector) {
      if (vector->length + 1 >= vector->capacity) {
         int check = _vectorReserve(vector, _calcBuffer(vector));
         if (check == 0) { return 0; }
      }

      T current = *vectorGet(vector, index);
      vectorSet(vector, index, value);

      for (int i = index + 1; i <= vector->length + 1; i++) {
         T next = *vectorGet(vector, i);
         _vectorSetOutside(vector, i, current);
         current = next;
      }
      vector->length += 1;
      return 1;
   }

   return 0;
}

template <typename T>
int vectorErase(Vector<T>* vector, int index) {
   if (index < 0 || index > vector->length) {
      return 0;
   }
   if (index == vector->length) { vector->length -= 1; }

   if (vector) {
      for (int i = index; i < vector->length; i++) {
         T next = *vectorGet(vector, i + 1);
         vectorSet(vector, i, next);
      }
      vector->length -= 1;
      return 1;
   }
   return 0;
}

template <typename T>
int vectorEmplace(Vector<T>* vector, int index, int value) {
   //todo emplace should initialize the object and add it
   return 0;
}

template <typename T>
int vectorClear(Vector<T>* vector) {
   if (vector) {
      vector->length = 0;
      return 1;
   }
   return 0;
}

//template <typename T>
//int vectorFind(Vector<T>* vector, T value) {
//   for (int i = 1; i <= vector->length; i++) {
//      T found = *vectorGet(vector, i);
//      if (found == value) {
//         return i;
//      }
//   }
//   return 0;
//}