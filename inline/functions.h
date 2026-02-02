//
// Created by justin on 2/1/26.
//

#include <iostream>

/*

  *
 ***
*****
 ***
  *

*/

// x is odd
inline void foo(int x) {
    int spaces = x / 2;
    for (int i = 0; i < x; i++) {
        int stars = x - (spaces*2);
        for (int j = 0; j < spaces; j++) std::cout << " ";
        for (int j = 0; j < stars; j++)  std::cout << "*";
        for (int j = 0; j < spaces; j++) std::cout << " ";
        std::cout << std::endl;

        if (i < x/2) spaces--;
        else spaces++;
    }
}

void bar(int x);
