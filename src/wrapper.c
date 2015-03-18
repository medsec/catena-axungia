#ifdef DBG
#include "catena-DBG.c"
#else
#include "catena-BRG.c"
#endif
#include "hash.h"

void wrapper(const uint8_t lambda, const uint8_t garlic){
	uint8_t x[H_LEN];
	uint8_t hv[H_LEN];
	uint8_t t[4];
	uint8_t c;

	//set values. compare to test-catena.c
	uint8_t min_garlic = garlic;
	uint8_t tweak_id = 0;
	uint8_t hashlen = H_LEN;
	uint8_t saltlen = 16;
	const uint8_t salt[16]=
    {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
     0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0XFF};
    uint8_t* pwd = malloc(8);
  	strncpy((char*)pwd, "Password", 8);
  	uint8_t pwdlen = 8;
  	const char *data  = "I'm a header";
  	uint8_t  datalen = strlen(data);
	uint8_t client = CLIENT;
	uint8_t hash[H_LEN];

	/*Compute H(V)*/
	__Hash1(VERSION_ID, strlen((char*)VERSION_ID), hv);

	/* Compute Tweak */
	t[0] = tweak_id;
	t[1] = lambda;
	t[2] = hashlen;
	t[3] = saltlen;

	/* Compute H(AD) */
	__Hash1((uint8_t *) data, datalen,x);

	/* Compute the initial value to hash  */
	__Hash5(hv, H_LEN, t, 4, x, H_LEN, pwd,  pwdlen, salt, saltlen, x);

	Flap(x, lambda, (min_garlic+1)/2, salt, saltlen, x);

	for(c=min_garlic; c <= garlic; c++)
	{
	  Flap(x, lambda, c, salt, saltlen, x);
	  if( (c==garlic) && (client == CLIENT))
	  {
	    memcpy(hash, x, H_LEN);
	    // return 0;
	  }
	  __Hash2(&c,1, x,H_LEN, x);
	  memset(x+hashlen, 0, H_LEN-hashlen);
	}
	memcpy(hash, x, hashlen);

}