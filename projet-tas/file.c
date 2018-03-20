   1 #include "global.h"
   2 #include "compute.h"
   3 #include "graphics.h"
   4 #include "debug.h"
   5 #include "ocl.h"
   6 #include "scheduler.h"
   7 
   8 #include <stdbool.h>
   9 
  10 static long unsigned int *TABLE=NULL;
  11 
  12 static volatile int changement ;
  13 
  14 static unsigned long int max_grains;
  15 
  16 #define table(i,j) TABLE[(i)*DIM+(j)]
  17 
  18 #define RGB(r,v,b) (((r)<<24|(v)<<16|(b)<<8))
  19 
  20 void sable_init()
  21 {
  22   TABLE=calloc(DIM*DIM,sizeof(long unsigned int));
  23 }
  24 
  25 void sable_finalize()
  26  {
  27    free(TABLE);
  28  }
  29 
  30 
  31 ///////////////////////////// Production d'une image
  32 void sable_refresh_img()
  33 {
  34   unsigned long int max = 0;
  35     for (int i = 1; i < DIM-1; i++)
  36       for (int j = 1; j < DIM-1; j++)
  37         {
  38           int g =table(i,j);
  39           int r,v,b;
  40           r = v = b = 0;
  41             if ( g == 1)
  42               v=255;
  43             else if (g == 2)
  44               b = 255;
  45             else if (g == 3)
  46               r = 255;
  47             else if (g == 4)
  48               r = v = b = 255 ;
  49             else if (g > 4)
  50               r = b = 255 - (240 * ((double) g) / (double) max_grains);
  51 
  52             cur_img (i,j) = RGB(r,v,b);
  53             if (g > max)
  54               max = g;
  55         }
  56     max_grains = max;
  57 }
  58 
  59 
  60 
  61 ///////////////////////////// Configurations initiales
  62 
  63 static void sable_draw_4partout(void);
  64 
  65 void sable_draw (char *param)
  66 {
  67   char func_name [1024];
  68   void (*f)(void) = NULL;
  69 
  70   sprintf (func_name, "draw_%s", param);
  71   f = dlsym (DLSYM_FLAG, func_name);
  72 
  73   if (f == NULL) {
  74     printf ("Cannot resolve draw function: %s\n", func_name);
  75     f = sable_draw_4partout;
  76   }
  77 
  78   f ();
  79 }
  80 
  81 
  82 void sable_draw_4partout(void){
  83   max_grains = 8;
  84   for (int i=1; i < DIM-1; i++)
  85     for(int j=1; j < DIM-1; j++)
  86       table (i, j) = 4;
  87 }
  88 
  89 
  90 void sable_draw_DIM(void){
  91   max_grains = DIM;
  92    for (int i=DIM/4; i < DIM-1; i+=DIM/4)
  93      for(int j=DIM/4; j < DIM-1; j+=DIM/4)
  94        table (i, j) = i*j/4;
  95 }
  96 
  97 
  98  void sable_draw_alea(void){
  99    max_grains = DIM;
 100  for (int i= 0; i < DIM>>2; i++)
 101    {
 102      table (1+random() % (DIM-2) , 1+ random() % (DIM-2)) = random() % (DIM);
 103    }
 104 }
 105 
 106 
 107 ///////////////////////////// Version séquentielle simple (seq)
 108 
 109 static inline void compute_new_state (int y, int x)
 110 {
 111   if (table(y,x) >= 4)
 112     {
 113       unsigned long int div4 = table(y,x) / 4;
 114       table(y,x-1)+=div4;
 115       table(y,x+1)+=div4;
 116       table(y-1,x)+=div4;
 117       table(y+1,x)+=div4;
 118       table(y,x)%=4;
 119       changement = 1;
 120     }
 121 }
 122 
 123 static inline void compute_new_state2 (int y, int x)
 124 {
 125   if(x==0 || y==0 || x==DIM-1 || y==DIM-1){
 126     changement=1;
 127     return;
 128   }
 129   if (table(y,x) >= 4)
 130     {
 131       unsigned long int div4 = table(y,x) / 4;
 132       table(y,x+1)+=div4;
 133       table(y-1,x)+=div4;
 134       table(y+1,x)+=div4;
 135       table(y,x)%=4;
 136       changement = 1;
 137     }
 138 }
 139 
 140 
 141 static void traiter_tuile_loop_unroll4 (int i_d, int j_d, int i_f, int j_f)
 142 {
 143   PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
 144 
 145   int mod = (j_f - j_d + 1) % 5;
 146 
 147   for (int i = i_d; i <= i_f; i++){
 148     for (int j = j_d; j <= (j_f - mod); j+=5){
 149       compute_new_state (i, j);
 150       compute_new_state(i,j+1);
 151       compute_new_state(i, j+2);
 152       compute_new_state(i,j+3);
 153       compute_new_state(i,j+4);
 154     }
 155   for(int j = (j_f - mod); j <= j_f; ++j)
 156     compute_new_state(i,j);
 157   }
 158 }
 159 
 160 ///////////////////////////// Boucle déroulé j -> j+3
 161 static void traiter_tuile_loop_unroll3 (int i_d, int j_d, int i_f, int j_f)
 162 {
 163   PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
 164 
 165   int mod = (j_f - j_d + 1) % 4;
 166 
 167   for (int i = i_d; i <= i_f; i++){
 168     for (int j = j_d; j <= (j_f - mod); j+=4){
 169       compute_new_state (i, j);
 170       compute_new_state(i,j+1);
 171       compute_new_state(i, j+2);
 172       compute_new_state(i,j+3);
 173     }
 174     for(int j = (j_f - mod); j <= j_f; ++j)
 175       compute_new_state(i,j);
 176   }
 177 }
 178 
 179 ///////////////////////////// Boucle déroulé j -> j+2
 180 static void traiter_tuile_loop_unroll2 (int i_d, int j_d, int i_f, int j_f)
 181 {
 182   PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
 183 
 184   int mod = (j_f - j_d + 1) % 3;
 185 
 186   for (int i = i_d; i <= i_f; i++){
 187     for (int j = j_d; j <= (j_f - mod); j+=3){
 188       compute_new_state (i, j);
 189       compute_new_state(i,j+1);
 190       compute_new_state(i, j+2);
 191     }
 192     for(int j = (j_f - mod); j <= j_f; ++j)
 193       compute_new_state(i,j);
 194   }
 195 }
 196 
 197 ///////////////////////////// Boucle déroulé j -> j+1
 198 static void traiter_tuile_loop_unroll1 (int i_d, int j_d, int i_f, int j_f)
 199 {
 200   PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
 201 
 202   int mod = (j_f - j_d + 1) % 2;
 203 
 204   for (int i = i_d; i <= i_f; i++){
 205     for (int j = j_d; j <= (j_f - mod); j+=2){
 206       compute_new_state (i, j);
 207       compute_new_state(i,j+1);
 208     }
 209     for(int j = (j_f - mod); j <= j_f; ++j)
 210       compute_new_state(i,j);
 211   }
 212 }
 213 
 214 //////////////////////////// Traintement de initial de tuile
 215 static void traiter_tuile (int i_d, int j_d, int i_f, int j_f)
 216 {
 217   PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
 218 
 219   for (int i = i_d; i <= i_f; i++)
 220     for (int j = j_d; j <= j_f; j++)
 221       compute_new_state (i, j);
 222 
 223 }
 224 
 225 static void traiter_tuile_omp_for (int i_d, int j_d, int i_f, int j_f)
 226 {
 227   PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
 228   int reali;
 229   int realj;
 230   for(int k=0; k<9; k++){
 231     #pragma omp for
 232     for (int i = i_d; i <= i_f; i+=3){
 233       reali=i+k%3;
 234       if(reali<=i_f){
 235         for (int j = j_d; j <= j_f; j+=3){
 236           realj=j+k/3;
 237           if(realj<=j_f){
 238             compute_new_state (reali, realj);
 239           }
 240         }
 241 
 242       }
 243     }
 244   }
 245 }
 246 
 247 static void traiter_tuile_omp_task (int i_d, int j_d, int i_f, int j_f)
 248 {
 249   PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
 250   int reali;
 251   int realj;
 252   int km1;
 253   #pragma omp single
 254   for(int k=0; k<9; k++){
 255     km1=k-1;
 256     for (int i = i_d; i <= i_f; i+=3){
 257       reali=i+k%3;
 258       if(reali<=i_f){
 259         for (int j = j_d; j <= j_f; j+=3){
 260           realj=j+k/3;
 261           if(realj<=j_f){
 262             if(k==0){
 263               #pragma omp task firstprivate(reali, realj) depend(out:k)
 264               compute_new_state (reali, realj);
 265             }else{
 266               #pragma omp task firstprivate(reali, realj) depend(in:km1) depend(out:k)
 267               compute_new_state (reali, realj);
 268             }
 269           }
 270         }
 271 
 272       }
 273     }
 274   }
 275   #pragma omp taskwait
 276 }
 277 
 278 #pragma GCC push_options
 279 #pragma GCC optimize("unroll-all-loops")
 280 //////////////////////////// Traintement de initial de tuile
 281 static void traiter_tuile_loop_unrollfull (int i_d, int j_d, int i_f, int j_f)
 282 {
 283   PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
 284 
 285   for (int i = i_d; i <= i_f; i++)
 286     for (int j = j_d; j <= j_f; j++)
 287       compute_new_state (i, j);
 288 
 289 }
 290 #pragma GCC pop_options
 291 
 292 unsigned sable_compute_omp_for (unsigned nb_iter)
 293 {
 294   int reali;
 295   int realj;
 296   int it;
 297   changement=1;
 298   #pragma omp parallel
 299   for (it = 1; it <= nb_iter; it ++) {
 300     changement = 0;
 301     // On itére sur les coordonnées des tuiles
 302     for(int i=0; i<9; i++){
 303       #pragma omp for
 304       for (int j=1; j <= (DIM-2); j+=3){
 305         reali=j+i%3;
 306         if(reali<=DIM-2){
 307           for (int k=1; k <= (DIM-2); k+=3){
 308             realj=k+i/3;
 309             if(realj<=DIM-2){
 310               compute_new_state(reali, realj);
 311             }
 312           }
 313         }
 314       }
 315     }
 316   }
 317 
 318   return (changement==0)?it:0;
 319 }
 320 
 321 
 322 // Renvoie le nombre d'itérations effectuées avant stabilisation, ou 0
 323 unsigned sable_compute_seq (unsigned nb_iter)
 324 {
 325 
 326   for (unsigned it = 1; it <= nb_iter; it ++) {
 327     changement = 0;
 328     // On traite toute l'image en un coup (oui, c'est une grosse tuile)
 329     traiter_tuile (1, 1, DIM - 2, DIM - 2);
 330     if(changement == 0)
 331       return it;
 332   }
 333 
 334   return 0;
 335 }
 336 
 337 unsigned sable_compute_omp (unsigned nb_iter)
 338 {
 339   int it;
 340   #pragma omp parallel
 341   for (it = 1; it <= nb_iter; it ++) {
 342     changement = 0;
 343     // On traite toute l'image en un coup (oui, c'est une grosse tuile)
 344     traiter_tuile_omp_task (1, 1, DIM - 2, DIM - 2);
 345   }
 346   return (changement==0)?it:0;
 347 }
 348 
 349 // Renvoie le nombre d'itérations effectuées avant stabilisation, ou 0
 350 unsigned sable_compute_seq_full_unroll (unsigned nb_iter)
 351 {
 352 
 353   for (unsigned it = 1; it <= nb_iter; it ++) {
 354     changement = 0;
 355     // On traite toute l'image en un coup (oui, c'est une grosse tuile)
 356     traiter_tuile_loop_unrollfull (1, 1, DIM - 2, DIM - 2);
 357     if(changement == 0)
 358       return it;
 359   }
 360   return 0;
 361 }
 362 
 363 
 364 // Renvoie le nombre d'itérations effectuées avant stabilisation, ou 0
 365 // utilise le traitement de tuile dont la boucle à été déroulé une fois
 366 unsigned sable_compute_seq_unroll1 (unsigned nb_iter)
 367 {
 368 
 369   for (unsigned it = 1; it <= nb_iter; it ++) {
 370     changement = 0;
 371     // On traite toute l'image en un coup (oui, c'est une grosse tuile)
 372     traiter_tuile_loop_unroll1 (1, 1, DIM - 2, DIM - 2);
 373     if(changement == 0)
 374       return it;
 375   }
 376   return 0;
 377 }
 378 
 379 // Renvoie le nombre d'itérations effectuées avant stabilisation, ou 0
 380 // utilise le traitement de tuile dont la boucle à été déroulé deux fois
 381 unsigned sable_compute_seq_unroll2 (unsigned nb_iter)
 382 {
 383 
 384   for (unsigned it = 1; it <= nb_iter; it ++) {
 385     changement = 0;
 386     // On traite toute l'image en un coup (oui, c'est une grosse tuile)
 387     traiter_tuile_loop_unroll2 (1, 1, DIM - 2, DIM - 2);
 388     if(changement == 0)
 389       return it;
 390   }
 391   return 0;
 392 }
 393 
 394 // Renvoie le nombre d'itérations effectuées avant stabilisation, ou 0
 395 // utilise le traitement de tuile dont la boucle à été déroulé trois fois
 396 unsigned sable_compute_seq_unroll3 (unsigned nb_iter)
 397 {
 398 
 399   for (unsigned it = 1; it <= nb_iter; it ++) {
 400     changement = 0;
 401     // On traite toute l'image en un coup (oui, c'est une grosse tuile)
 402     traiter_tuile_loop_unroll3 (1, 1, DIM - 2, DIM - 2);
 403     if(changement == 0)
 404       return it;
 405   }
 406   return 0;
 407 }
 408 
 409 // Renvoie le nombre d'itérations effectuées avant stabilisation, ou 0
 410 // utilise le traitement de tuile dont la boucle à été déroulé quatre fois
 411 unsigned sable_compute_seq_unroll4 (unsigned nb_iter)
 412 {
 413 
 414   for (unsigned it = 1; it <= nb_iter; it ++) {
 415     changement = 0;
 416     // On traite toute l'image en un coup (oui, c'est une grosse tuile)
 417     traiter_tuile_loop_unroll4 (1, 1, DIM - 2, DIM - 2);
 418     if(changement == 0)
 419       return it;
 420   }
 421   return 0;
 422 }
 423 
 424 ///////////////////////////// Version séquentielle tuilée (tiled)
 425 
 426 
 427 static unsigned tranche = 0;
 428 //////////////////////////// Simple tiled Version
 429 unsigned sable_compute_tiled (unsigned nb_iter)
 430 {
 431   tranche = DIM / GRAIN;
 432 
 433   for (unsigned it = 1; it <= nb_iter; it ++) {
 434     changement = 0;
 435     // On itére sur les coordonnées des tuiles
 436     for (int i=0; i < GRAIN; i++)
 437       for (int j=0; j < GRAIN; j++)
 438         {
 439           traiter_tuile (i == 0 ? 1 : (i * tranche) /* i debut */,
 440                          j == 0 ? 1 : (j * tranche) /* j debut */,
 441                          (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
 442                          (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
 443         }
 444 
 445     if(changement == 0)
 446       return it;
 447   }
 448 
 449   return 0;
 450 }
 451 
 452 unsigned sable_compute_tiled_omp_for (unsigned nb_iter)
 453 {
 454   tranche = DIM / GRAIN;
 455   int reali;
 456   int realj;
 457   int it;
 458   changement=1;
 459   #pragma omp parallel
 460   for (it = 1; it <= nb_iter; it ++) {
 461     changement = 0;
 462     // On itére sur les coordonnées des tuiles
 463     for(int i=0; i<4; i++){
 464       #pragma omp for collapse(2) firstprivate(changement, reali, realj)
 465       for (int j=0; j < GRAIN/2; j++){
 466         for (int k=0; k < GRAIN/2; k++){
 467           reali=j*2+i%2;
 468           realj=k*2+i/2;
 469           traiter_tuile_loop_unroll1(reali == 0 ? 1 : (reali * tranche) /* i debut */,
 470                          realj == 0 ? 1 : (realj * tranche) /* j debut */,
 471                          (reali + 1) * tranche - 1 - (reali == GRAIN-1)/* i fin */,
 472                          (realj + 1) * tranche - 1 - (realj == GRAIN-1)/* j fin */);
 473         }
 474       }
 475     }
 476   }
 477 
 478   return (changement==0)?it:0;
 479 }
 480 
 481 unsigned sable_compute_tiled_omp_task (unsigned nb_iter)
 482 {
 483   tranche = DIM / GRAIN;
 484   int reali;
 485   int realj;
 486   int it;
 487   int im1;
 488   changement=1;
 489   #pragma omp parallel
 490   for (it = 1; it <= nb_iter; it ++) {
 491     // On itére sur les coordonnées des tuiles
 492     #pragma omp single
 493     {
 494     changement = 0;
 495     for(int i=0; i<4; i++){
 496       im1=i-1;
 497       for (int j=0; j < GRAIN/2; j++){
 498         for (int k=0; k < GRAIN/2; k++){
 499           reali=j*2+i%2;
 500           realj=k*2+i/2;
 501           if(i==0){
 502             #pragma omp task firstprivate(reali, realj, changement) depend(out:i)
 503             {
 504               traiter_tuile_loop_unroll1(reali == 0 ? 1 : (reali * tranche) /* i debut */,
 505                                  realj == 0 ? 1 : (realj * tranche) /* j debut */,
 506                                  (reali + 1) * tranche - 1 - (reali == GRAIN-1)/* i fin */,
 507                                  (realj + 1) * tranche - 1 - (realj == GRAIN-1)/* j fin */);
 508             }
 509 
 510           }else{
 511             #pragma omp task firstprivate(reali, realj, changement) depend(in : im1) depend(out:i)
 512             {
 513               traiter_tuile_loop_unroll1(reali == 0 ? 1 : (reali * tranche) /* i debut */,
 514               realj == 0 ? 1 : (realj * tranche) /* j debut */,
 515               (reali + 1) * tranche - 1 - (reali == GRAIN-1)/* i fin */,
 516               (realj + 1) * tranche - 1 - (realj == GRAIN-1)/* j fin */);
 517             }
 518           }
 519         }
 520       }
 521     }
 522     }
 523     #pragma omp taskwait
 524   }
 525 
 526   return (changement==0)?it:0;
 527 }
 528 
 529 // -1 rien
 530 // 0 elle seul
 531 // 1 gauche
 532 // 2 haut
 533 // 4 droite
 534 // 8 bas
 535 static int test_tuile (int i_d, int j_d, int i_f, int j_f)
 536 {
 537   int res=-1;
 538   int passH=0;
 539   int passD=0;
 540   int passG=0;
 541   int passB=0;
 542 
 543   PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
 544 
 545   for (int i = i_d; i <= i_f; i++){
 546     for (int j = j_d; j <= j_f; j++){
 547       if(table(i,j)>=4){
 548         if(i==i_d && !passG){
 549           passG=!passG;
 550           res+=1;
 551         }else if(i==i_f && !passD){
 552           passD=!passD;
 553           res+=4;
 554         }else{
 555           res+=0;
 556         }
 557         if(j==j_d && !passH){
 558           passH=!passH;
 559           res+=2;
 560         }else if(j==j_f && !passB){
 561           passB=!passB;
 562           res+=8;
 563         }else{
 564           res+=0;
 565         }
 566       }
 567     }
 568   }
 569   return res;
 570 }
 571 
 572 static int test_tuile2 (int i_d, int j_d, int i_f, int j_f)
 573 {
 574   for (int i = i_d; i <= i_f; i++){
 575     for (int j = j_d; j <= j_f; j++){
 576       if(table(i,j)>=4){
 577         return 1;
 578       }
 579     }
 580   }
 581   return 0;
 582 }
 583 
 584 int needModification(int i, int j, int** mod){
 585   int needMod=0;
 586   if(i>0){
 587     if(i<GRAIN-1){
 588       if(j>0){
 589         if(j<GRAIN-1 && (mod[i][j-1]!=0 || mod[i][j+1]!=0 || mod[i+1][j]!=0 || mod[i-1][j]!=0)){
 590           needMod=1;
 591         }else{
 592           if(mod[i-1][j]!=0 || mod[i+1][j]!=0 || mod[i][j-1]!=0){
 593             needMod=1;
 594           }
 595         }
 596       }else{
 597         if(mod[i-1][j]!=0 || mod[i+1][j]!=0 || mod[i][j+1]!=0){
 598           needMod=1;
 599         }
 600       }
 601     }else{
 602       if(j>0){
 603         if(j<GRAIN-1 && (mod[i][j-1]!=0 || mod[i][j+1]!=0 ||  mod[i-1][j]!=0)){
 604           needMod=1;
 605         }else{
 606           if(mod[i-1][j]!=0 || mod[i][j-1]!=0){
 607             needMod=1;
 608           }
 609         }
 610       }else{
 611         if(mod[i-1][j]!=0 || mod[i][j+1]!=0){
 612           needMod=1;
 613         }
 614       }
 615     }
 616   }else{
 617     if(j>0){
 618       if(j<GRAIN-1 && (mod[i][j-1]!=0 || mod[i][j+1]!=0 ||  mod[i+1][j]!=0)){
 619         needMod=1;
 620       }else{
 621         if(mod[i+1][j]!=0 || mod[i][j-1]!=0){
 622           needMod=1;
 623         }
 624       }
 625     }else{
 626       if(mod[i+1][j]!=0 || mod[i][j+1]!=0){
 627         needMod=1;
 628       }
 629     }
 630   }
 631   return needMod;
 632 }
 633 
 634 unsigned sable_compute_tiled_opt (unsigned nb_iter)
 635 {
 636   tranche = DIM / GRAIN;
 637   int mod[GRAIN][GRAIN];
 638   int prec[GRAIN][GRAIN];
 639   for(int i=0; i<GRAIN*GRAIN; ++i){
 640     mod[i/GRAIN][i%GRAIN]=0;
 641     prec[i/GRAIN][i%GRAIN]=test_tuile2(i/GRAIN == 0 ? 1 : (i/GRAIN * tranche) /* i debut */,
 642       i%GRAIN == 0 ? 1 : (i%GRAIN * tranche) /* j debut */,
 643       (i/GRAIN + 1) * tranche - 1 - (i/GRAIN == GRAIN-1)/* i fin */,
 644       (i%GRAIN + 1) * tranche - 1 - (i%GRAIN == GRAIN-1)/* j fin */);
 645   }
 646   int needMod=0;
 647   int res;
 648   for (unsigned it = 1; it <= nb_iter; it ++) {
 649     changement = 0;
 650     // On itére sur les coordonnées des tuiles
 651     for (int i=0; i < GRAIN; i++){
 652       for (int j=0; j < GRAIN; j++){
 653         //if(prec[i][j]==0){
 654         //  needMod=needModification(i,j, prec);
 655         //}
 656         for(int k=0; k<GRAIN*GRAIN; ++k){
 657           mod[k/GRAIN][k%GRAIN]=0;
 658         }
 659         if(mod[i][j]==0 && (prec[i][j]!=0 || needMod)){
 660           res=test_tuile(i == 0 ? 1 : (i * tranche) /* i debut */,
 661                             j == 0 ? 1 : (j * tranche) /* j debut */,
 662                             (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
 663                             (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
 664           if(res==-1){
 665             mod[i][j]=0;
 666           }else{
 667 
 668             traiter_tuile_loop_unroll1 (i == 0 ? 1 : (i * tranche) /* i debut */,
 669                             j == 0 ? 1 : (j * tranche) /* j debut */,
 670                             (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
 671                             (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
 672             mod[i][j]=1;
 673             if(mod[i][j+1]==0 && res>=8 && j<GRAIN-1){ //modif sur le bas de la tuile
 674               res-=8;
 675               traiter_tuile_loop_unroll1 (i == 0 ? 1 : (i * tranche) /* i debut */,
 676                     (j+1) == 0 ? 1 : ((j+1) * tranche) /* j debut */,
 677                     (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
 678                     ((j+1) + 1) * tranche - 1 - ((j+1) == GRAIN-1)/* j fin */);
 679               mod[i][j+1]=1;
 680             }
 681             if(mod[i+1][j]==0 && res>=4 && i<GRAIN-1){ //modif sur la droite de la tuile
 682               res-=4;
 683               traiter_tuile_loop_unroll1 ((i+1) == 0 ? 1 : ((i+1) * tranche) /* i debut */,
 684                     j == 0 ? 1 : (j * tranche) /* j debut */,
 685                     ((i+1) + 1) * tranche - 1 - ((i+1) == GRAIN-1)/* i fin */,
 686                     (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
 687               mod[i+1][j]=1;
 688             }
 689             if(mod[i][j-1]==0 && res>=2 && j>0){ //modif sur le haut de la tuile
 690               res-=2;
 691               traiter_tuile_loop_unroll1 (i == 0 ? 1 : (i * tranche) /* i debut */,
 692                     (j-1) == 0 ? 1 : ((j-1) * tranche) /* j debut */,
 693                     (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
 694                     ((j-1) + 1) * tranche - 1 - ((j-1) == GRAIN-1)/* j fin */);
 695               mod[i][j-1]=1;
 696             }
 697             if(mod[i-1][j]==0 && res>=1 && i>0){ //modif sur la gauche de la tuile
 698               res-=1;
 699               traiter_tuile_loop_unroll1 ((i-1) == 0 ? 1 : ((i-1) * tranche) /* i debut */,
 700                     j == 0 ? 1 : (j * tranche) /* j debut */,
 701                     ((i-1) + 1) * tranche - 1 - ((i-1) == GRAIN-1)/* i fin */,
 702                     (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
 703               mod[i-1][j]=1;
 704             }
 705 
 706           }
 707 
 708         }
 709             }
 710     }
 711     for (int i=0; i < GRAIN; i++){
 712       for (int j=0; j < GRAIN; j++){
 713         prec[i][j]=mod[i][j];
 714       }
 715     }
 716     if(changement == 0){
 717       return it;
 718     }
 719   }
 720   return 0;
 721 }
 722 
 723 void traiter_tuile_max(int i, int j, int** mod, int** prec){
 724   tranche=DIM/GRAIN;
 725   int needMod;
 726   if(prec[i][j]==0){
 727     needMod=needModification(i, j, prec);
 728   }else{
 729     needMod=1;
 730   }
 731   if(needMod){
 732     mod[i][j]=test_tuile2(i == 0 ? 1 : (i * tranche) /* i debut */,
 733       j == 0 ? 1 : (j * tranche) /* j debut */,
 734       (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
 735       (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);;
 736   }else{
 737     mod[i][j]=0;
 738   }
 739   if(mod[i][j]!=0){
 740     traiter_tuile(i == 0 ? 1 : (i * tranche) /* i debut */,
 741       j == 0 ? 1 : (j * tranche) /* j debut */,
 742       (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
 743       (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
 744   }
 745 
 746 
 747 }
 748 
 749 unsigned sable_compute_tiled_test(unsigned nb_iter){
 750   tranche=DIM/GRAIN;
 751   int** prec=malloc(GRAIN*sizeof(int*));
 752   int** mod=malloc(GRAIN*sizeof(int*));
 753   for(int i=0; i<GRAIN; ++i){
 754     mod[i]=malloc(GRAIN*sizeof(int));
 755     prec[i]=malloc(GRAIN*sizeof(int));
 756     for(int j=0; j<GRAIN; j++){
 757       mod[i][j]=0;
 758       prec[i][j]=1;
 759     }
 760   }
 761   int reali=0;
 762   int realj=0;
 763   for (unsigned it = 1; it <= nb_iter; it ++) {
 764 
 765       changement = 0;
 766       for(int i=0; i<4; i++){
 767         for (int j=0; j < GRAIN/2; j++){
 768           for (int k=0; k < GRAIN/2; k++){
 769             reali=j*2+i%2;
 770             realj=k*2+i/2;
 771             traiter_tuile_max(reali, realj, mod, prec);
 772           }
 773         }
 774       }
 775 
 776       for (int i=0; i < GRAIN; i++){
 777         for (int j=0; j < GRAIN; j++){
 778           prec[i][j]=mod[i][j];
 779         }
 780       }
 781       if(changement == 0){
 782         return it;
 783       }
 784   }
 785   return 0;
 786 
 787 }
 788 unsigned sable_compute_tiled_test_omp(unsigned nb_iter){
 789   int** prec=malloc(GRAIN*sizeof(int*));
 790   int** mod=malloc(GRAIN*sizeof(int*));
 791   #pragma omp parallel for
 792   for(int i=0; i<GRAIN; ++i){
 793     mod[i]=malloc(GRAIN*sizeof(int));
 794     prec[i]=malloc(GRAIN*sizeof(int));
 795     for(int j=0; j<GRAIN; j++){
 796       mod[i][j]=0;
 797       prec[i][j]=1;
 798     }
 799   }
 800   int reali=0;
 801   int realj=0;
 802   int it;
 803   int im1;
 804   changement=1;
 805   #pragma omp parallel
 806   for (it = 1; it <= nb_iter; it ++) {
 807 
 808       #pragma omp single
 809       {
 810 
 811       changement = 0;
 812       for(int i=0; i<4; i++){
 813         im1=i-1;
 814         for (int j=0; j < GRAIN/2; j++){
 815           for (int k=0; k < GRAIN/2; k++){
 816             reali=j*2+i%2;
 817             realj=k*2+i/2;
 818             if(i==0){
 819               #pragma omp task firstprivate(reali, realj, changement) depend(out: i)
 820               traiter_tuile_max(reali, realj, mod, prec);
 821             }else{
 822               #pragma omp task firstprivate(reali, realj, changement) depend(in:im1) depend(out: i)
 823               traiter_tuile_max(reali, realj, mod, prec);
 824             }
 825           }
 826         }
 827       }
 828       }
 829       #pragma omp taskwait
 830       #pragma omp for collapse(2)
 831       for (int i=0; i < GRAIN; i++){
 832         for (int j=0; j < GRAIN; j++){
 833           prec[i][j]=mod[i][j];
 834         }
 835       }
 836   }
 837   return (changement==0)?it:0;
 838 
 839 }
 840 
 841 unsigned sable_compute_tiled_test_omp_for(unsigned nb_iter){
 842   tranche=DIM/GRAIN;
 843   int** prec=malloc(GRAIN*sizeof(int*));
 844   int** mod=malloc(GRAIN*sizeof(int*));
 845   #pragma omp parallel for
 846   for(int i=0; i<GRAIN; ++i){
 847     mod[i]=malloc(GRAIN*sizeof(int));
 848     prec[i]=malloc(GRAIN*sizeof(int));
 849     for(int j=0; j<GRAIN; j++){
 850       mod[i][j]=0;
 851       prec[i][j]=1;
 852     }
 853   }
 854   int reali=0;
 855   int realj=0;
 856   int it;
 857   changement=1;
 858   #pragma omp parallel
 859   for (it = 1; it <= nb_iter; it ++) {
 860 
 861     changement = 0;
 862       {
 863 
 864       for(int i=0; i<4; i++){
 865         #pragma omp for collapse(2) firstprivate(changement, reali, realj)
 866         for (int j=0; j < GRAIN/2; j++){
 867           for (int k=0; k < GRAIN/2; k++){
 868             reali=j*2+i%2;
 869             realj=k*2+i/2;
 870             traiter_tuile_max(reali, realj, mod, prec);
 871           }
 872         }
 873       }
 874       }
 875       #pragma omp for collapse(2) firstprivate(changement, reali, realj)
 876       for (int i=0; i < GRAIN; i++){
 877         for (int j=0; j < GRAIN; j++){
 878           prec[i][j]=mod[i][j];
 879         }
 880       }
 881   }
 882   return (changement==0)?it:0;
 883 
 884 }
 885 
 886 //////////////////////////////////////////////////////////////Correspond au second tableau du sujet
 887 unsigned sable_compute_tiled_task_v2 (unsigned nb_iter)
 888 {
 889   tranche = DIM / GRAIN;
 890   int it;
 891   changement=1;
 892   int mod = GRAIN % 3;
 893 
 894 #pragma omp parallel
 895 #pragma omp single
 896   for (it = 1; it <= nb_iter && changement != 0; it ++) {
 897     changement = 0;
 898     int j = 0;
 899 
 900     // On itére sur les coordonnées des tuiles
 901     for (int i=0; i < GRAIN - mod; i+=3){
 902 
 903 #pragma omp task firstprivate(i, changement)  depend(out : table(i,j))
 904       for (j=0; j < GRAIN ; j++)
 905         traiter_tuile_loop_unroll1 (i == 0 ? 1 : (i * tranche) /* i debut */,
 906                        j == 0 ? 1 : (j * tranche) /* j debut */,
 907                        (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
 908                        (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
 909 #pragma omp task firstprivate(i, changement) depend(in: table(i,j)) depend(out: table(i+1,j))
 910       if(((i + 1) * tranche) < DIM)
 911         for (j=0; j < GRAIN; j++)
 912           traiter_tuile_loop_unroll1 (((i + 1) * tranche) /* i debut */,
 913                          j == 0 ? 1 : (j * tranche) /* j debut */,
 914                          (i + 2) * tranche - 1 - (i == GRAIN-1)/* i fin */,
 915                          (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
 916 #pragma omp task firstprivate(i, changement) depend(in: table(i+1,j)) depend(out: table(i+2,j))
 917       if(((i + 2) * tranche) < DIM)
 918         for (j=0; j < GRAIN; j++)
 919           traiter_tuile_loop_unroll1 (((i + 2) * tranche) /* i debut */,
 920                          j == 0 ? 1 : (j * tranche) /* j debut */,
 921                          (i + 3) * tranche - 1 - (i == GRAIN-1)/* i fin */,
 922                          (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
 923     }
 924 
 925     for(int i = GRAIN - mod; i < GRAIN; ++i)
 926       for(j = 0; j < GRAIN; ++j)
 927         traiter_tuile_loop_unroll1 (i == 0 ? 1 : (i * tranche) /* i debut */,
 928                        j == 0 ? 1 : (j * tranche) /* j debut */,
 929                        (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
 930                        (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
 931 
 932   }
 933 
 934   return (changement==0)?it:0;
 935 }
 936 
 937 //Le grain doit être un petit multiple de dim pour une version bien optimisée
 938 unsigned sable_compute_tiled_task_v1 (unsigned nb_iter)
 939 {
 940   tranche = DIM / GRAIN;
 941   int it;
 942   changement=1;
 943 #pragma omp parallel
 944 #pragma omp single
 945   for (it = 1; it <= nb_iter && changement != 0; it ++) {
 946     changement = 0;
 947 
 948     int mod = GRAIN%3;
 949     // On itére sur les coordonnées des tuiles
 950     for (int i=0; i < GRAIN - mod; i+=3){
 951       for (int j=0; j < GRAIN - mod; j+=3){
 952 #pragma omp task firstprivate(i,j, changement)  \
 953   depend(out : table(i,j))
 954         traiter_tuile_loop_unroll1 (i == 0 ? 1 : (i * tranche) /* i debut */,
 955                        j == 0 ? 1 : (j * tranche) /* j debut */,
 956                        (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
 957                        (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
 958 
 959         if(((i + 1) * tranche) < DIM)
 960 #pragma omp task firstprivate(i,j, changement) depend(in: table(i,j)) \
 961   depend(out: table(i+1,j))
 962           traiter_tuile_loop_unroll1 (((i + 1) * tranche) /* i debut */,
 963                          j == 0 ? 1 : (j * tranche) /* j debut */,
 964                          (i + 2) * tranche - 1 - (i == GRAIN-1)/* i fin */,
 965                          (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
 966 
 967         if(((i + 2) * tranche) < DIM)
 968 #pragma omp task firstprivate(i,j, changement) depend(in: table(i+1,j)) \
 969   depend(out: table(i+2,j))
 970           traiter_tuile_loop_unroll1 (((i + 2) * tranche) /* i debut */,
 971                          j == 0 ? 1 : (j * tranche) /* j debut */,
 972                          (i + 3) * tranche - 1 - (i == GRAIN-1)/* i fin */,
 973                          (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
 974 
 975         if((j + 1)*tranche < DIM)
 976 #pragma omp task firstprivate(i,j, changement) depend(in: table(i,j)) \
 977   depend(out: table(i,j+1))
 978           traiter_tuile_loop_unroll1 (i == 0 ? 1 : (i * tranche) /* i debut */,
 979                          ((j+1) * tranche) /* j debut */,
 980                          (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
 981                          (j + 2) * tranche - 1 - (j == GRAIN-1)/* j fin */);
 982 
 983         if((j + 2)*tranche < DIM)
 984 #pragma omp task firstprivate(i,j, changement) depend(in: table(i,j+1)) \
 985   depend(out: table(i,j+2))
 986           traiter_tuile_loop_unroll1 (i == 0 ? 1 : (i * tranche) /* i debut */,
 987                          ((j+2) * tranche) /* j debut */,
 988                          (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
 989                          (j + 3) * tranche - 1 - (j == GRAIN-1)/* j fin */);
 990 
 991         if((j + 1)*tranche < DIM && (i+1) * tranche < DIM)
 992 #pragma omp task firstprivate(i,j, changement) depend(in: table(i+1,j), table(i,j+1)) \
 993   depend(out: table(i+1,j+1))
 994           traiter_tuile_loop_unroll1 (((i + 1) * tranche) /* i debut */,
 995                          ((j+1) * tranche) /* j debut */,
 996                          (i + 2) * tranche - 1 - (i == GRAIN-1)/* i fin */,
 997                          (j + 2) * tranche - 1 - (j == GRAIN-1)/* j fin */);
 998 
 999         if( (j+1)*tranche < DIM && (i+2)*tranche < DIM)
1000 #pragma omp task firstprivate(i,j, changement) depend(in: table(i+1,j+1), table(i,j+2)) \
1001   depend(out: table(i+2,j+1))
1002           traiter_tuile_loop_unroll1 (((i + 2) * tranche) /* i debut */,
1003                          ((j+1) * tranche) /* j debut */,
1004                          (i + 3) * tranche - 1 - (i == GRAIN-1)/* i fin */,
1005                          (j + 2) * tranche - 1 - (j == GRAIN-1)/* j fin */);
1006 
1007         if((j + 2)*tranche < DIM && (i+1) * tranche < DIM)
1008 #pragma omp task firstprivate(i,j, changement) depend(in: table(i+1,j+1), table(i+2,j)) \
1009   depend(out: table(i+2,j+1))
1010           traiter_tuile_loop_unroll1 (((i + 1) * tranche) /* i debut */,
1011                          ((j+2) * tranche) /* j debut */,
1012                          (i + 2) * tranche - 1 - (i == GRAIN-1)/* i fin */,
1013                          (j + 3) * tranche - 1 - (j == GRAIN-1)/* j fin */);
1014 
1015         if((j + 2)*tranche < DIM && (i+2) * tranche < DIM)
1016 #pragma omp task firstprivate(i,j, changement) depend(in: table(i+1,j+2), table(i+2,j+1)) \
1017   depend(out: table(i+2,j+2))
1018           traiter_tuile_loop_unroll1 (((i + 2) * tranche) /* i debut */,
1019                          ((j+2) * tranche) /* j debut */,
1020                          (i + 3) * tranche - 1 - (i == GRAIN-1)/* i fin */,
1021                          (j + 3) * tranche - 1 - (j == GRAIN-1)/* j fin */);
1022       }
1023 
1024       for(int k = i ;  k < i + 3; ++k)
1025           for(int j = GRAIN  - mod; j < GRAIN; ++j)
1026             traiter_tuile ( k == 0 ? 1 : (k * tranche) /* i debut */,
1027                             j == 0 ? 1 : (j * tranche) /* j debut */,
1028                             (k + 1) * tranche - 1 - (k == GRAIN-1)/* i fin */,
1029                             (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
1030 
1031     }
1032 
1033 
1034       for(int i = GRAIN - mod; i < GRAIN; ++i)
1035         for(int j = 0; j < GRAIN; ++j)
1036           traiter_tuile ((i * tranche) /* i debut */,
1037                          (j * tranche) /* j debut */,
1038                          (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
1039                          (j + 1)* tranche - 1 - (j == GRAIN-1)/* j fin */);
1040 
1041   }
1042 
1043   return (changement==0)?it:0;
1044 }
1045 
1046 ////////////////////////////////////////////////////////Version sequentiel tuilé utilisant loop_unroll1
1047 unsigned sable_compute_tiled_unroll1 (unsigned nb_iter)
1048 {
1049   tranche = DIM / GRAIN;
1050 
1051   for (unsigned it = 1; it <= nb_iter; it ++) {
1052     changement = 0;
1053     // On itére sur les coordonnées des tuiles
1054     for (int i=0; i < GRAIN; i++)
1055       for (int j=0; j < GRAIN; j++)
1056         {
1057           traiter_tuile_loop_unroll1 (i == 0 ? 1 : (i * tranche) /* i debut */,
1058                          j == 0 ? 1 : (j * tranche) /* j debut */,
1059                          (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
1060                          (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
1061         }
1062     if(changement == 0)
1063       return it;
1064   }
1065 
1066   return 0;
1067 }
1068 
1069 ////////////////////////////////////////////////////////Version sequentiel tuilé utilisant loop_unroll2
1070 unsigned sable_compute_tiled_unroll2 (unsigned nb_iter)
1071 {
1072   tranche = DIM / GRAIN;
1073 
1074   for (unsigned it = 1; it <= nb_iter; it ++) {
1075     changement = 0;
1076     // On itére sur les coordonnées des tuiles
1077     for (int i=0; i < GRAIN; i++)
1078       for (int j=0; j < GRAIN; j++)
1079         {
1080           traiter_tuile_loop_unroll2 (i == 0 ? 1 : (i * tranche) /* i debut */,
1081                          j == 0 ? 1 : (j * tranche) /* j debut */,
1082                          (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
1083                          (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
1084         }
1085     if(changement == 0)
1086       return it;
1087   }
1088 
1089   return 0;
1090 }
1091 
1092 ////////////////////////////////////////////////////////Version sequentiel tuilé utilisant loop_unroll3
1093 unsigned sable_compute_tiled_unroll3 (unsigned nb_iter)
1094 {
1095   tranche = DIM / GRAIN;
1096 
1097   for (unsigned it = 1; it <= nb_iter; it ++) {
1098     changement = 0;
1099     // On itére sur les coordonnées des tuiles
1100     for (int i=0; i < GRAIN; i++)
1101       for (int j=0; j < GRAIN; j++)
1102         {
1103           traiter_tuile_loop_unroll3 (i == 0 ? 1 : (i * tranche) /* i debut */,
1104                          j == 0 ? 1 : (j * tranche) /* j debut */,
1105                          (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
1106                          (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
1107         }
1108     if(changement == 0)
1109       return it;
1110   }
1111 
1112   return 0;
1113 }
1114 
1115 ////////////////////////////////////////////////////////Version sequentiel tuilé utilisant loop_unroll4
1116 unsigned sable_compute_tiled_unroll4 (unsigned nb_iter)
1117 {
1118   tranche = DIM / GRAIN;
1119 
1120   for (unsigned it = 1; it <= nb_iter; it ++) {
1121     changement = 0;
1122     // On itére sur les coordonnées des tuiles
1123     for (int i=0; i < GRAIN; i++)
1124       for (int j=0; j < GRAIN; j++)
1125         {
1126           traiter_tuile_loop_unroll4 (i == 0 ? 1 : (i * tranche) /* i debut */,
1127                          j == 0 ? 1 : (j * tranche) /* j debut */,
1128                          (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
1129                          (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
1130         }
1131     if(changement == 0)
1132       return it;
1133   }
1134 
1135   return 0;
1136 }
1137 ////////////////////////////////////////////////////////Version sequentiel tuilé utilisant loop_unrollfull
1138 unsigned sable_compute_tiled_full_unroll (unsigned nb_iter)
1139 {
1140   tranche = DIM / GRAIN;
1141 
1142   for (unsigned it = 1; it <= nb_iter; it ++) {
1143     changement = 0;
1144     // On itére sur les coordonnées des tuiles
1145     for (int i=0; i < GRAIN; i++)
1146       for (int j=0; j < GRAIN; j++)
1147         {
1148           traiter_tuile_loop_unrollfull (i == 0 ? 1 : (i * tranche) /* i debut */,
1149                          j == 0 ? 1 : (j * tranche) /* j debut */,
1150                          (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
1151                          (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
1152         }
1153     if(changement == 0)
1154       return it;
1155   }
1156 
1157   return 0;
1158 }
1159 
1160 ///////////////////////////// Version utilisant un ordonnanceur maison (sched)
1161 
1162 unsigned P;
1163 
1164 void sable_init_sched ()
1165 {
1166   sable_init();
1167   P = scheduler_init (-1);
1168 }
1169 
1170 void sable_finalize_sched ()
1171 {
1172   sable_finalize();
1173   scheduler_finalize ();
1174 }
1175 
1176 static inline void *pack (int i, int j)
1177 {
1178   uint64_t x = (uint64_t)i << 32 | j;
1179   return (void *)x;
1180 }
1181 
1182 static inline void unpack (void *a, int *i, int *j)
1183 {
1184   *i = (uint64_t)a >> 32;
1185   *j = (uint64_t)a & 0xFFFFFFFF;
1186 }
1187 
1188 static inline unsigned cpu (int i, int j)
1189 {
1190   return 1;
1191 }
1192 
1193 static inline void create_task (task_func_t t, int i, int j)
1194 {
1195   scheduler_create_task (t, pack (i, j), cpu (i, j));
1196 }
1197 
1198 //////// First Touch
1199 
1200 static void zero_seq (int i_d, int j_d, int i_f, int j_f)
1201 {
1202 
1203   for (int i = i_d; i <= i_f; i++)
1204     for (int j = j_d; j <= j_f; j++)
1205       next_img (i, j) = cur_img (i, j) = 0 ;
1206 }
1207 
1208 static void first_touch_task (void *p, unsigned proc)
1209 {
1210   int i, j;
1211 
1212   unpack (p, &i, &j);
1213 
1214   //PRINT_DEBUG ('s', "First-touch Task is running on tile (%d, %d) over cpu #%d\n", i, j, proc);
1215   zero_seq (i * tranche, j * tranche, (i + 1) * tranche - 1, (j + 1) * tranche - 1);
1216 }
1217 
1218 void sable_ft_sched (void)
1219 {
1220   tranche = DIM / GRAIN;
1221 
1222   for (int i = 0; i < GRAIN; i++)
1223     for (int j = 0; j < GRAIN; j++)
1224       create_task (first_touch_task, i, j);
1225 
1226   scheduler_task_wait ();
1227 }
1228 
1229 //////// Compute
1230 
1231 static void compute_task (void *p, unsigned proc)
1232 {
1233   int i, j;
1234 
1235   unpack (p, &i, &j);
1236 
1237   //PRINT_DEBUG ('s', "Compute Task is running on tile (%d, %d) over cpu #%d\n", i, j, proc);
1238           traiter_tuile (i == 0 ? 1 : (i * tranche) /* i debut */,
1239                          j == 0 ? 1 : (j * tranche) /* j debut */,
1240                          (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
1241                          (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
1242 }
1243 
1244 unsigned sable_compute_sched (unsigned nb_iter)
1245 {
1246   tranche = DIM / GRAIN;
1247 
1248   for (unsigned it = 1; it <= nb_iter; it ++) {
1249     changement=0;
1250     for (int i = 0; i < GRAIN; i++)
1251       for (int j = 0; j < GRAIN; j++)
1252         create_task (compute_task, i, j);
1253 
1254     scheduler_task_wait ();
1255 
1256     if (changement == 0)
1257       return it;
1258   }
1259 
1260   return 0;
1261 }
papRebufatAudoyRSSAtom