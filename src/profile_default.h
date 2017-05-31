#ifndef PROFILE_DEFAULT_H
#define PROFILE_DEFAULT_H


uint8_t *default_profile[6][13];

void init_default_profile() {

  //no string pressed
  default_profile[UNDEF][UNDEF] = acc_zero;

  //vybrnkavaci struna
  default_profile[0][0]  = acc_F_tone;
  default_profile[0][1]  = acc_Fis_tone;
  default_profile[0][2]  = acc_G_tone;
  default_profile[0][3]  = acc_Gis_tone;
  default_profile[0][4]  = acc_A_tone;
  default_profile[0][5]  = acc_AisB_tone;
  default_profile[0][6]  = acc_H_tone;
  default_profile[0][7]  = acc_C_tone;
  default_profile[0][8]  = acc_Cis_tone;
  default_profile[0][9]  = acc_D_tone;
  default_profile[0][10] = acc_Dis_tone;
  default_profile[0][11] = acc_E_tone;
  default_profile[0][12] = acc_F2_tone;

  //durova
  default_profile[1][0]  = acc_Czm;
  default_profile[1][1]  = acc_EsDur;
  default_profile[1][2]  = acc_Bdur;
  default_profile[1][3]  = acc_Fdur;
  default_profile[1][4]  = acc_Cdur;
  default_profile[1][5]  = acc_Gdur;
  default_profile[1][6]  = acc_Ddur;
  default_profile[1][7]  = acc_Adur;
  default_profile[1][8]  = acc_Edur;
  default_profile[1][9]  = acc_Hdur;
  default_profile[1][10] = acc_FisDur;
  default_profile[1][11] = acc_CisDur;
  default_profile[1][12] = acc_CisDur;

  //sedmickova
  default_profile[2][0]  = acc_CisZm;
  default_profile[2][1]  = acc_Czv;
  default_profile[2][2]  = acc_B7;
  default_profile[2][3]  = acc_F7;
  default_profile[2][4]  = acc_C7;
  default_profile[2][5]  = acc_G7;
  default_profile[2][6]  = acc_D7;
  default_profile[2][7]  = acc_A7;
  default_profile[2][8]  = acc_E7;
  default_profile[2][9]  = acc_H7;
  default_profile[2][10] = acc_Fmoll;
  default_profile[2][11] = acc_Cis7;
  default_profile[2][12] = acc_Cis7;

  //petkova
  default_profile[3][0]  = acc_CisZm;
  default_profile[3][1]  = acc_Czv;
  default_profile[3][2]  = acc_B5;
  default_profile[3][3]  = acc_F5;
  default_profile[3][4]  = acc_C5;
  default_profile[3][5]  = acc_G5;
  default_profile[3][6]  = acc_D5;
  default_profile[3][7]  = acc_A5;
  default_profile[3][8]  = acc_E5;
  default_profile[3][9]  = acc_H5;
  default_profile[3][10] = acc_Fmoll;
  default_profile[3][11] = acc_Cis5;
  default_profile[3][12] = acc_Cis5;

  //custom
  default_profile[4][0]  = acc_Dzm;
  default_profile[4][1]  = acc_Cmoll;
  default_profile[4][2]  = acc_Gmoll;
  default_profile[4][3]  = acc_Dmoll;
  default_profile[4][4]  = acc_Amoll;
  default_profile[4][5]  = acc_Emoll;
  default_profile[4][6]  = acc_Hmoll;
  default_profile[4][7]  = acc_FShMin;
  default_profile[4][8]  = acc_CShMin;
  default_profile[4][9]  = acc_GShMin7;
  default_profile[4][10] = acc_Dzv;
  default_profile[4][11] = acc_DisZv;
  default_profile[4][12] = acc_DisZv;
  
  //molova
  default_profile[5][0]  = acc_Dzm;
  default_profile[5][1]  = acc_Cmoll;
  default_profile[5][2]  = acc_Gmoll;
  default_profile[5][3]  = acc_Dmoll;
  default_profile[5][4]  = acc_Amoll;
  default_profile[5][5]  = acc_Emoll;
  default_profile[5][6]  = acc_Hmoll;
  default_profile[5][7]  = acc_FisMoll;
  default_profile[5][8]  = acc_CisMoll;
  default_profile[5][9]  = acc_CisZv;
  default_profile[5][10] = acc_Dzv;
  default_profile[5][11] = acc_DisZv;
  default_profile[5][12] = acc_DisZv;

}

#endif
