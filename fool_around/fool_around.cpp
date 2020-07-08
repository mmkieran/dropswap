// fool_around.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <stdlib.h>
#include <unordered_map>


//I really recommend having your own struct that looks like this!
struct Vec2
{
   float x, y;
};


//4x4 matrix - a standard for graphics.
struct Mat4x4
{
   float values[16];
};

//This will be used to translate, rotate, and scale our meshes
Mat4x4 identityMatrix() {
   Mat4x4 identity;

   for (int row = 0; row < 4; row++) {
      for (int col = 0; col < 4; col++) {
         if (row == col) {
            identity.values[4 * row + col] = 1;
         }
         else {
            identity.values[4 * row + col] = 0;
         }
      }
   }

   return identity;
}

//translation
Mat4x4 translateMatrix(Vec2 movement) {
   Mat4x4 out = identityMatrix();
   out.values[3] = movement.x;
   out.values[7] = movement.y;

   return out;
}

//scale
Mat4x4 scaleMatrix(Vec2 scale) {
   Mat4x4 out = identityMatrix();
   out.values[0] = scale.x;
   out.values[5] = scale.y;
   return out;
}

Mat4x4 rotateMatrix(float degreeAngle) {
   Mat4x4 out = identityMatrix();

   float PI = 3.1415926535;
   float radianAngle = PI / 180.0 * degreeAngle;

   out.values[0] = cosf(radianAngle);
   out.values[1] = -sinf(radianAngle);
   out.values[4] = sinf(radianAngle);
   out.values[5] = cosf(radianAngle);

   return out;
}

Mat4x4 multiplyMatrix(Mat4x4 left, Mat4x4 right) {
   Mat4x4 out;

   //loop through left matrix
   for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
         float sum = 0;
         for (int k = 0; k < 4; k++) {
            sum += left.values[4 * i + k] * right.values[4 * k + j];
         }
         out.values[4 * i + j] = sum;
      }
   }
   return out;
}

//translate, rotate, scale together
Mat4x4 transformMatrix(Vec2 movement, float degreeAngle, Vec2 scale) {
   Mat4x4 intermediate = multiplyMatrix(translateMatrix(movement), rotateMatrix(degreeAngle));
   Mat4x4 out = multiplyMatrix(intermediate, scaleMatrix(scale));
   return out;
}

void printMatrix(Mat4x4 test) {

   for (int row = 0; row < 4; row++) {
      for (int col = 0; col < 4; col++) {
         printf("%f, ", test.values[4 * row + col]);
      }
      printf("\n");
   }
   printf("\n");
}

int main()
{
   float a = 1.1;
   int b = 22;

   float c = a + b;

   printf("%f", c);
   if (a + b > 23.05f) {
      printf("True");
   }
}