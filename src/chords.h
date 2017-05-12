#ifndef CHORDS_H
#define CHORDS_H

#include <Arduino.h>
//                         E  A  D  G  B  Ehi
uint8_t acc_zero[6]     = {52,57,62,67,71,76};
//vybrnkavani
//                         F  A# D# G# C  F
uint8_t acc_F_tone[6]   = {53,58,63,68,72,77};
//                         F# D  E  A  C# F#
uint8_t acc_Fis_tone[6] = {54,59,64,69,73,78};
uint8_t acc_G_tone[6]   = {55,60,65,70,74,79};
uint8_t acc_Gis_tone[6] = {56,61,66,71,75,80};
uint8_t acc_A_tone[6]   = {57,62,67,72,76,81};
uint8_t acc_AisB_tone[6]= {58,63,68,73,77,82};
uint8_t acc_H_tone[6]   = {59,64,69,74,78,83};
uint8_t acc_C_tone[6]   = {60,65,70,75,79,84};
uint8_t acc_Cis_tone[6] = {61,66,71,76,80,85};
uint8_t acc_D_tone[6]   = {62,67,72,77,81,86};
uint8_t acc_Dis_tone[6] = {63,68,73,78,82,87};
uint8_t acc_E_tone[6]   = {64,69,74,79,83,88};
uint8_t acc_Eis_tone[6] = {65,70,75,80,84,89};

uint8_t acc_Czm[6]     = {0 ,57,63,69,72,78};
uint8_t acc_EsDur[6]   = {0 ,63,67,70,75,79};
uint8_t acc_Bdur[6]    = {53,58,65,70,74,77};
uint8_t acc_Fdur[6]    = {53,60,65,69,72,77};
uint8_t acc_Cdur[6]    = {0 ,60,64,67,72,76};
uint8_t acc_Gdur[6]    = {55,59,62,67,71,79};
uint8_t acc_Ddur[6]    = {0 ,57,62,69,74,78};
uint8_t acc_Adur[6]    = {52,57,64,69,73,76};
uint8_t acc_Edur[6]    = {52,59,64,68,71,76};
uint8_t acc_Hdur[6]    = {54,59,66,71,75,78};
uint8_t acc_FisDur[6]  = {54,61,66,70,73,78};
uint8_t acc_CisDur[6]  = {56,61,68,73,77,80};

uint8_t acc_CisZm[6]   = {52,58,64,67,73,76};
uint8_t acc_Czv[6]     = {52,60,64,68,72,76};
uint8_t acc_B7[6]      = {53,58,65,68,74,77};
uint8_t acc_F7[6]      = {0 ,57,65,69,72,76};
uint8_t acc_C7[6]      = {52,60,64,70,72,76};
uint8_t acc_G7[6]      = {55,59,62,67,71,77};
uint8_t acc_D7[6]      = {0 ,57,62,69,72,78};
uint8_t acc_A7[6]      = {52,57,64,69,73,79};
uint8_t acc_E7[6]      = {52,59,64,68,74,76};
uint8_t acc_H7[6]      = {54,59,66,69,75,78};
uint8_t acc_Fmoll[6]   = {53,60,65,68,72,77};
uint8_t acc_Cis7[6]    = {56,61,68,71,77,80};

uint8_t acc_B5[6]      = {53,58,65,68,74,77};
uint8_t acc_F5[6]      = {0 ,57,65,69,72,76};
uint8_t acc_C5[6]      = {52,60,64,70,72,76};
uint8_t acc_G5[6]      = {55,59,62,67,71,77};
uint8_t acc_D5[6]      = {0 ,57,62,69,72,78};
uint8_t acc_A5[6]      = {52,57,64,69,73,79};
uint8_t acc_E5[6]      = {52,59,64,68,74,76};
uint8_t acc_H5[6]      = {54,59,66,69,75,78};
uint8_t acc_Cis5[6]    = {56,61,68,71,77,80};

uint8_t acc_Dzm[6]     = {0 ,0 ,62,68,71,77};
uint8_t acc_Cmoll[6]   = {55,60,67,72,75,79};
uint8_t acc_Gmoll[6]   = {55,62,67,70,74,79};
uint8_t acc_Dmoll[6]   = {0 ,57,62,69,74,77};
uint8_t acc_Amoll[6]   = {52,57,64,69,72,76};
uint8_t acc_Emoll[6]   = {52,59,64,67,71,76};
uint8_t acc_Hmoll[6]   = {54,59,66,71,74,78};
uint8_t acc_FisMoll[6] = {54,61,66,69,73,78};
uint8_t acc_CisMoll[6] = {56,61,68,73,76,80};
uint8_t acc_CisZv[6]   = {0 ,57,65,69,73,77};
uint8_t acc_Dzv[6]     = {0 ,0 ,66,70,74,78};
uint8_t acc_DisZv[6]   = {0 ,0, 67,71,75,79};


#endif
