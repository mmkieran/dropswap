#include <math.h>
#include <stdlib.h>
#include <iostream>
#include "mymath.h"

double mathTrunc(double val, int places) {
   int tmp = val * pow(10, places);
   return (double)tmp / pow(10, places);
}

Vec2 getXYDistance(Vec2 pos, int angle, double dist) {
   Vec2 out = { 0, 0 };
   float PI = 3.1415926535f;
   float radianAngle = PI / 180.0f * (angle - 90);  //-90 Because 90 is up in the right angle triangle

   out.x = cos(radianAngle) * dist;
   out.y = sin(radianAngle) * dist;

   return out;
}

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

   float PI = 3.1415926535f;
   float radianAngle = PI / 180.0f * degreeAngle;

   out.values[0] = cosf(radianAngle);
   out.values[1] = -sinf(radianAngle);
   out.values[4] = sinf(radianAngle);
   out.values[5] = cosf(radianAngle);

   return out;
}

Mat4x4 multiplyMatrix(Mat4x4 left, Mat4x4 right) {
   Mat4x4 out;

   for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
         float sum = 0;
         for (int k = 0; k < 4; k++) {
            //This is confusing, but you use the third index to increment columns for left and rows in right
            //I made it row major order... sadness
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

////TESTING MATRIX STUFF
//Mat4x4 identity = identityMatrix();
//printMatrix(identity);
//
//Mat4x4 scale = scaleMatrix({ 1.5, 1.5 });
//printMatrix(scale);
//
//Mat4x4 rotate = rotateMatrix(45.0);
//printMatrix(rotate);
//
//Mat4x4 translate = translateMatrix({ 4, 6 });
//printMatrix(translate);