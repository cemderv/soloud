#pragma once

class resonator
{
  public:
    /* Convert formant freqencies and bandwidth into resonator difference equation coefficents
     */
    void initResonator(int aFrequency, /* Frequency of resonator in Hz  */
                       int aBandwidth, /* Bandwidth of resonator in Hz  */
                       int aSamplerate);

    /* Convert formant freqencies and bandwidth into anti-resonator difference equation constants
     */
    void initAntiresonator(int aFrequency, /* Frequency of resonator in Hz  */
                           int aBandwidth, /* Bandwidth of resonator in Hz  */
                           int aSamplerate);

    /* Set gain */
    void setGain(float aG);

    /* Generic resonator function */
    float resonate(float input);

    /* Generic anti-resonator function
       Same as resonator except that a,b,c need to be set with initAntiresonator()
       and we save inputs in p1/p2 rather than outputs.
       There is currently only one of these - "mNasalZero"

       Output = (mNasalZero.a * input) + (mNasalZero.b * oldin1) + (mNasalZero.c * oldin2)
     */

    float antiresonate(float input);

  private:
    float mA = 0.0f;
    float mB;
    float mC  = 0.0f;
    float mP1 = 0.0f;
    float mP2 = 0.0f;
};
