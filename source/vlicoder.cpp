#include <stdlib.h>
// #include <stdio.h>
#include "vlicoder.h"
#include "vliprecalc.h"


/* -----------------------------------------------
	VLI encoder function
	----------------------------------------------- */
vli_code* encode_vli( int val ) {
	vli_code* vli = ( vli_code* ) calloc( 1, sizeof( vli_code ) );
	
	if ( vli == NULL ) return NULL;	
	if ( val < ( 1 << (VLI_PREC-1) ) ) {
		vli->len = vli_precalc[ val ].len;
		vli->res = vli_precalc[ val ].res;
	} else {
		int len;
		int res;
		for ( len = VLI_PREC, res = val, val >>= VLI_PREC; val > 0; val >>= 1, len++ );
		if ( len > 0 ) res ^= 1 << (len-1);
		vli->len = len;
		vli->res = res;
	}
	
	return vli;
}


/* -----------------------------------------------
	VLI decoder function
	----------------------------------------------- */
int decode_vli( vli_code* vli ) {
	return ( vli->len <= 1 ) ? vli->len : ( 1 << (vli->len-1) ) | vli->res; 
}


/*int main( int argc, char** argv ) {
	vli_code* vli;
	
	for ( int i = 0; i < ( 1 << 16 ); i++ ) {
		vli = encode_vli( i );
		fprintf( stdout, "{ %2i, 0x%.4X }, ", vli->len, vli->res );
		if ( (i+1) % 4 == 0 ) fprintf( stdout, "\n" );
		free( vli );
	}
}*/
