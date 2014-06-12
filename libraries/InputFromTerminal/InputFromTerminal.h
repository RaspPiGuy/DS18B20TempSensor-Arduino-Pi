/* Input Number from Terminal
Accepts inputs from terminal until Enter is pressed


termFloat() Accepts only numbers, minus sign, period, and backspace keys
are recognized - all other entries ask user to reenter.

Assures that only one decimal point is included

Assures that the minus sign can only be inputted as
the first character.

Returns float.

termInt() Accepts only Integers.  All other entries ask user to reenter.

mjl - thepiandi.blogspot.com  May 25, 2014
*/

#ifndef TERM_INPUT_H
#define TERM_INPUT_H

#include "Arduino.h"

class TERM_INPUT{
  public:
    TERM_INPUT();
    float termFloat();
    long termInt();

};

#endif


