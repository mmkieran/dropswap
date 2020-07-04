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
   Mat4x4 identity = identityMatrix();
   printMatrix(identity);

   Vec2 scaleValue = { 2, 2 };
   Mat4x4 scale = scaleMatrix(scaleValue);
   printMatrix(scale);

   float degreeAngle = 45.0;
   Mat4x4 rotate = rotateMatrix(degreeAngle);
   printMatrix(rotate);

   Vec2 movement = { 1, 1 };
   Mat4x4 translate = translateMatrix(movement);
   printMatrix(translate);

   Mat4x4 multiply = multiplyMatrix(scale, rotate);
   printMatrix(multiply);

   //Mat4x4 transform = transformMatrix(movement, degreeAngle, scaleValue);
   //printMatrix(transform);
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
