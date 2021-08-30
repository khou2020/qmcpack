
// Generated with convert_hdf_to_cpp.py
// clang-format off

void get_diamond_fill_data(qmcplusplus::FillData &fd)
{
  fd.numSamples = 10;
  fd.numParam = 12;
  fd.sum_wgt =              10;
  fd.sum_e_wgt =    -103.7499619;
  fd.sum_esq_wgt =     1078.787276;
  fd.reweight = {
               1,                1,                1,                1,                1,                1,                1,                1,                1,                1, };
  fd.energy_new = {
     -10.9357659,     -10.46149593,     -11.29403468,     -9.602985419,     -10.36807556,     -10.45821967,     -9.909916465,      -10.1359362,     -10.69485531,     -9.888676774, };
  fd.derivRecords.resize(10, 12);
  double tmp_fd_derivRecords[] = {
  // Row 0
  -0.2286003136,    -2.600971305,    -2.356612193,    -1.703136176,  -0.01027311999,   -0.6120254734,    -2.683287408,    -2.165833879,  -0.01032177422,   -0.4117882157, 
     -0.590000738,    -1.611614628, 
  // Row 1
  -0.2448053671,    -2.094904985,    -2.782051055,    -1.391172142,               0,   -0.1212447633,    -1.177773317,    -2.367085146,   -0.1009323659,   -0.8952401509, 
    -0.9065906081,    -1.401874825, 
  // Row 2
 -0.04014696778,    -1.218688802,    -3.468742508,    -3.021455479,               0,  -0.05734335292,    -1.259951146,    -3.687096023,               0,  -0.06329273765, 
     -1.293574691,      -3.0359952, 
  // Row 3
              0,  -0.08381984715,    -1.671550141,    -5.688903771,               0,               0,   -0.1165718769,    -1.587150947,   -0.0544090681,    -1.332168546, 
     -2.062846679,    -2.011874739, 
  // Row 4
              0,  -0.08553263259,    -1.899814737,    -5.507612941,               0,               0,   -0.1506428241,   -0.8372391734, -0.009545251787,   -0.6897896872, 
     -1.372221283,    -2.016620905, 
  // Row 5
              0,   -0.3366370894,    -2.572204445,    -3.141726828,               0,               0,  -0.03103830195,    -1.471394595,               0, -0.008867579278, 
     -0.866575278,    -3.668164232, 
  // Row 6
-0.002945665771,   -0.8146938323,    -3.380868615,    -4.008806017,  -0.01902827846,   -0.4700350571,   -0.7880533534,    -2.461237427,               0,  -0.05628373311, 
      -1.24138019,    -3.276309528, 
  // Row 7
-0.001867617963,   -0.3314154591,    -1.901634985,    -3.472668675,               0,               0,   -0.1479267639,     -2.06190926,               0, -0.008627597385, 
    -0.8794966052,    -4.322215903, 
  // Row 8
-0.002893325832,   -0.6753940277,    -1.999420756,    -2.623104718,               0,               0,   -0.2962103431,    -2.774351967,   -0.4196553807,   -0.5888231024, 
    -0.8078547389,    -3.124939472, 
  // Row 9
  -0.2284720461,   -0.9282029207,    -1.104616242,    -2.792995416,               0,               0,   -0.1449693873,    -1.873185186,               0, -0.0001898731144, 
     -0.604320504,    -4.450790088, 
  };
  std::copy(tmp_fd_derivRecords, tmp_fd_derivRecords+120, fd.derivRecords.begin());
  fd.HDerivRecords.resize(10, 12);
  double tmp_fd_HDerivRecords[] = {
  // Row 0
    3.343010687,    -14.99752856,     8.726976103,     2.478956299,      1.24263595,     1.398808292,    -21.74488966,     10.59469318,     1.300235608,    -4.522695553, 
      3.612520762,     10.18293224, 
  // Row 1
    1.093854496,     -8.81656263,     1.016547209,     11.72601702,              -0,     2.642984223,    -2.967090844,    -6.867646389,     1.505272459,    -9.231799667, 
       5.17809468,     21.41151715, 
  // Row 2
    1.472065228,   -0.4135093982,    -7.325582017,     6.441245404,              -0,     2.664246149,    0.7120330366,    -21.12482973,              -0,     4.083598095, 
     -3.461452953,     4.404921624, 
  // Row 3
             -0,     2.189729804,      6.20082365,    -18.49809103,              -0,              -0,     3.819651727,      -6.4129695,     4.144499519,     -12.7722003, 
     -1.948364008,     13.61235206, 
  // Row 4
             -0,     2.677769879,     3.902160349,    -11.61545621,              -0,              -0,     2.369971687,      3.10235051,     2.235422936,    -4.253038902, 
     -1.540025625,     9.896769912, 
  // Row 5
             -0,     4.935378333,    -10.43553565,     11.36511878,              -0,              -0,     4.039002473,     9.452720731,              -0,      1.96825472, 
     -5.764462722,     14.14327445, 
  // Row 6
   0.8132895982,     3.015210469,    -7.736831249,   -0.9904380953,    0.9861227503,    -7.694869664,     10.00841302,    -4.885399338,              -0,     4.221526708, 
    0.09150556107,     2.094428482, 
  // Row 7
   0.4488466521,     2.003489862,   0.04387730027,    -2.168258036,              -0,              -0,     6.480297181,  -0.07601405575,              -0,     2.376868479, 
      8.637378373,    -26.61488203, 
  // Row 8
   0.8167736401,     1.346673399,    -3.785065293,      12.3594022,              -0,              -0,     8.328829408,     -17.0159862,    -10.55252013,     8.299356206, 
      10.92759614,    -10.04185379, 
  // Row 9
   -1.411578233,     -1.51262086,     6.295582654,   -0.1427570437,              -0,              -0,     4.098449437,     3.622646633,              -0,    0.4251277044, 
       15.0783603,    -33.82207387, 
  };
  std::copy(tmp_fd_HDerivRecords, tmp_fd_HDerivRecords+120, fd.HDerivRecords.begin());
  fd.ovlp_gold.resize(13, 13);
  double tmp_fd_ovlp_gold[] = {
  // Row 0
              1,               0,               0,               0,               0,               0,               0,               0,               0,               0, 
                0,               0,               0, 
  // Row 1
              0,   0.01098098594,   0.06858716622,  -0.01039798251,  -0.09852612772, 2.076718323e-05,  0.007876253648,   0.04793356512,  0.007379526406, -0.001631625885, 
   0.001374308085,  -0.02421264274,  -0.02911190456, 
  // Row 2
              0,   0.06858716622,    0.6448458653,    0.1896038981,   -0.8771388418,  0.001535216455,    0.1142627618,    0.5808237994,    0.2566204327, -0.001840557885, 
  -0.007475554499,   -0.1741639416,   -0.3772867374, 
  // Row 3
              0,  -0.01039798251,    0.1896038981,    0.5075100743,   -0.2261912403,  0.002074571268,   0.06508241306,    0.2504179326,    0.3114030424,  -0.01230943561, 
   -0.07575745747,   0.02527075537,   -0.1595539251, 
  // Row 4
              0,  -0.09852612772,   -0.8771388418,   -0.2261912403,     1.811175175, -0.0003947600309,  -0.09358878778,   -0.6206670504,     -0.50724386,  -0.03630718292, 
      0.181989576,    0.4592107432,   0.02623566986, 
  // Row 5
              0, 2.076718323e-05,  0.001535216455,  0.002074571268, -0.0003947600309, 3.817551803e-05,  0.001153749225,  0.002264655728, 0.0006710608659, -0.0001636997417, 
  -0.0006580593466, -0.0001449853173, -0.0005841974494, 
  // Row 6
              0,  0.007876253648,    0.1142627618,   0.06508241306,  -0.09358878778,  0.001153749225,   0.04545731867,    0.1370911257,   0.02973628313, -0.005643672002, 
   -0.01205492015,  -0.02107367405,  -0.07754525846, 
  // Row 7
              0,   0.04793356512,    0.5808237994,    0.2504179326,   -0.6206670504,  0.002264655728,    0.1370911257,    0.6344432035,    0.2472562076,   -0.0125636242, 
  -0.003736243245,   -0.1031006283,   -0.4410493294, 
  // Row 8
              0,  0.007379526406,    0.2566204327,    0.3114030424,     -0.50724386, 0.0006710608659,   0.02973628313,    0.2472562076,    0.5477197807,   0.02536336069, 
   -0.08922853974,  -0.04831869379,   0.07029800426, 
  // Row 9
              0, -0.001631625885, -0.001840557885,  -0.01230943561,  -0.03630718292, -0.0001636997417, -0.005643672002,   -0.0125636242,   0.02536336069,   0.01540696797, 
    0.01795566159, -0.007008404045,  -0.01221296805, 
  // Row 10
              0,  0.001374308085, -0.007475554499,  -0.07575745747,     0.181989576, -0.0006580593466,  -0.01205492015, -0.003736243245,  -0.08922853974,   0.01795566159, 
     0.1931186628,    0.1083532318,   -0.3450309798, 
  // Row 11
              0,  -0.02421264274,   -0.1741639416,   0.02527075537,    0.4592107432, -0.0001449853173,  -0.02107367405,   -0.1031006283,  -0.04831869379, -0.007008404045, 
     0.1083532318,    0.1776227075,   -0.1399555968, 
  // Row 12
              0,  -0.02911190456,   -0.3772867374,   -0.1595539251,   0.02623566986, -0.0005841974494,  -0.07754525846,   -0.4410493294,   0.07029800426,  -0.01221296805, 
    -0.3450309798,   -0.1399555968,     1.070123297, 
  };
  std::copy(tmp_fd_ovlp_gold, tmp_fd_ovlp_gold+169, fd.ovlp_gold.begin());
  fd.ham_gold.resize(13, 13);
  double tmp_fd_ham_gold[] = {
  // Row 0
   -10.37499619,    0.6650525032,   -0.7542976704,   -0.1361152129,    0.7425576173,    0.2225669888,  -0.08010410401,     1.744311342,    -2.757235817,   -0.1260410692, 
    -0.9910952745,     3.005626645,    0.4098741998, 
  // Row 1
 0.007426296325,   -0.1419865282,   -0.1999859386,   -0.2568515226,    0.7290100671,  -0.01233341756,   -0.2001431369,    0.0653305427,   -0.3585208906,  -0.05598481887, 
     0.2061873998,  -0.06771747809,    0.3745851067, 
  // Row 2
   0.2028992998,    -1.401740461,    -2.361138243,    -3.275553247,     5.439698122,   -0.2155558679,    -1.928730167,   -0.7813024548,    -3.210288739,  -0.09114130603, 
       0.83286221,    0.7959330217,    0.7544248881, 
  // Row 3
   0.1735894811,   -0.3470576154,    -1.698505993,    -2.173448013,   -0.5469092862,   -0.1306522111,   -0.2544192478,    -2.253521962,   -0.7202518162,   0.09005170813, 
     -0.319953739,     2.315647846,    -4.755244376, 
  // Row 4
   -0.353016312,     1.741155535,     3.725420684,      1.80709266,    -7.579506033,    0.1409685703,     2.344932829,     1.163624997,     5.309817587,    -1.329928814, 
     0.1421508423,   -0.9703847897,    -2.031215928, 
  // Row 5
-0.0003088812004, -0.003400871983, -0.009529004909,  -0.01524630331,  0.007145743112,  -0.00288689897,  0.001024587002,     -0.01614855,  -0.01635641379, -3.100185339e-05, 
  0.0002240267966,   0.00685633049, -0.006106650111, 
  // Row 6
  0.01877899612,   -0.2692500005,   -0.4579932808,   -0.8178833113,    0.8547513721,   -0.1061975669,    -0.261756969,   -0.3800969613,   -0.8729611308,  -0.05450765494, 
     0.1604555643,    0.3482863813,  -0.09717777088, 
  // Row 7
   0.2298445957,    -1.353714436,    -2.101538113,    -3.816547997,     4.538988689,   -0.2835514148,       -1.946436,   -0.9625406253,    -3.164569505,   -0.2432987286, 
     0.7619022304,     1.415989029,   -0.0790683516, 
  // Row 8
   0.2038075986,   -0.4516497257,    -1.768294421,    -1.428349781,     1.414650505,  -0.04351033164,   -0.5127956251,    -2.519770554,    -0.446627594,      0.89156089, 
     -1.231474279,    -0.106147235,    0.5240616342, 
  // Row 9
  0.01066789174,  0.008137143886,  0.005444179366,    0.2228101506,  -0.08488765369,   0.01368078099,    0.0265526319,  -0.08488235489,    0.3660544621,    0.2294145211, 
    -0.4211321989,   -0.2429665432,    0.2575839201, 
  // Row 10
 -0.05059502334,  -0.05375657551,    0.4707725477,   -0.5619054279,   0.06834611846,   0.04002364487,   -0.1957311308,    0.5700963228,     1.701949487,   -0.5169914936, 
     0.1490203219,   -0.6575356071,   -0.5774429624, 
  // Row 11
 -0.07548840635,    0.3368054963,    0.6866747324,   -0.3160181654,    -1.740105368,    0.0427851212,    0.4115786947,   -0.0486462303,     1.871377481,   -0.5913846674, 
     0.1590382412,   -0.1864615399,    -1.697594193, 
  // Row 12
  -0.1168644233,     1.005147627,    0.7060533938,     4.076611747,     -2.06177559,    0.1280128741,     1.673241057,   -0.4433431022,    -1.081912359,     1.312011914, 
    -0.9460226834,    -1.108303516,     3.641651373, 
  };
  std::copy(tmp_fd_ham_gold, tmp_fd_ham_gold+169, fd.ham_gold.begin());
}

