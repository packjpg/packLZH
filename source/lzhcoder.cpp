#include <stdlib.h>
#include <string.h>
#include "bitops.h"
#include "lzhcoder.h"


/* -----------------------------------------------
	constructor for lzh reader class
	----------------------------------------------- */
	
lzh_encoder::lzh_encoder( iostream* source, int lim_count, int lim_dist ) {
	// setup lz encoder, 
	source->rewind();
	lz_enc = new lz_encoder( source, lim_count, lim_dist );
	// preset storage
	lz_store = NULL;
	vli_store = NULL;
	table_c = NULL;
	table_d = NULL;
	table_u = NULL;
}


/* -----------------------------------------------
	destructor for lzh reader class
	----------------------------------------------- */
	
lzh_encoder::~lzh_encoder( void ) {
	// cleanup storage, lz encoder
	cleanup_storage();
	delete ( lz_enc );
}


/* -----------------------------------------------
	lzh encode n symbols to destination
	----------------------------------------------- */
	
int lzh_encoder::encode_lzh( iostream* dest, int nsymb ) {
	huffman_writer* h_enc;
	lz_code** lzp;
	vli_code** vlip;
	unsigned char num[4];
	unsigned char* cdata;
	int csize;
	
	
	// allocate memory, check
	lzp = lz_store = ( lz_code** ) calloc( nsymb + 1, sizeof( lz_code* ) );
	vlip = vli_store = ( vli_code** ) calloc( nsymb + 1, sizeof( vli_code* ) );
	if ( ( lzp == NULL ) || ( vlip == NULL ) ) return -1;
	
	// lz/vli encoding loop
	for ( int i = 0; i < nsymb; i++, lzp++, vlip++ ) {
		*lzp = lz_enc->encode_lz(); if ( !(*lzp) ) return -1;
		*vlip = encode_vli( (*lzp)->dist ); if ( !(*vlip) ) return -1;
		if ( (*lzp)->count == 0 ) { nsymb = i; break; };
	}
	(*lzp) = NULL; (*vlip) = NULL;
	if ( nsymb == 0 ) return 0;
	
	// setup huffman tables
	setup_htables( dest );
	if ( ( !table_c ) || ( !table_d ) || ( !table_u ) ) return -1;
	
	// huffman encoding loop
	h_enc = new huffman_writer( 0 );
	lzp = lz_store; vlip = vli_store;
	for ( int i = 0; i < nsymb; i++, lzp++, vlip++ ) {
		h_enc->encode_symbol( table_c, (*lzp)->count );
		h_enc->encode_symbol( table_d, (*vlip)->len );
		h_enc->write_bits( (*vlip)->res, (*vlip)->len );
		if ( (*lzp)->dist == 0 ) for ( int j = 0; j < (*lzp)->count; j++ )
			h_enc->encode_symbol( table_u, (*lzp)->data[j] );
	}
	h_enc->encode_symbol( table_c, 0 );
	cdata = h_enc->getptr(); 
	csize = h_enc->getpos();
	delete ( h_enc );
	if ( ( !cdata ) && ( csize ) ) return -1;
	
	// csize, LSB first
	for ( int i = 0; i < 4; i++ )
		num[i] = ( unsigned char ) ( csize >> (8*(3-i)) ) & 0xFF;
	dest->write( (void*) num, 1, 4 );
	
	// write encoded data to file
	dest->write( (void*) cdata, 1, csize );
	
	// cleanup the storage
	free ( cdata );
	cleanup_storage();
	
	
	return csize;
}


/* -----------------------------------------------
	setup huffman tables
	----------------------------------------------- */
	
void lzh_encoder::setup_htables( iostream* dest ) {
	lz_code** lzp;
	vli_code** vlip;
	
	unsigned int counts_c[ 256 ];
	unsigned int counts_d[ 256 ];
	unsigned int counts_u[ 256 ];
	
	canonical_table* ctable_c;
	canonical_table* ctable_d;
	canonical_table* ctable_u;
	
	
	// set counts zero
	for ( int i = 0; i < 256; i++ )
		counts_c[i] = counts_d[i] = counts_u[i] = 0;
	counts_c[0] = 1; // end of stream symbol
	
	// count symbol occurences
	lzp = lz_store; vlip = vli_store;
	for ( lzp = lz_store, vlip = vli_store; *lzp; lzp++, vlip++ ) {
		counts_c[ (*lzp)->count ]++;
		counts_d[ (*vlip)->len ]++;
		if ( (*lzp)->dist == 0 ) for ( int j = 0; j < (*lzp)->count; j++ )
			counts_u[ (*lzp)->data[j] ]++;
	}
	
	// build canonical tables
	ctable_c = build_ctable( (unsigned int*) counts_c );
	ctable_d = build_ctable( (unsigned int*) counts_d );
	ctable_u = build_ctable( (unsigned int*) counts_u );
	// dangerous in case of out of memory, but won't happen anyways (!!!)
	
	// write canonical tables to the destination file
	dest->write( (void*) ctable_c->data, 1, ctable_c->total_size );
	dest->write( (void*) ctable_d->data, 1, ctable_d->total_size );
	dest->write( (void*) ctable_u->data, 1, ctable_u->total_size );
	
	// convert to hcode tables
	table_c = convert_ctable2hcode( ctable_c );
	table_d = convert_ctable2hcode( ctable_d );
	table_u = convert_ctable2hcode( ctable_u );
	
	// memory cleanup
	free( ctable_c->data ); free( ctable_c );
	free( ctable_d->data ); free( ctable_d );
	free( ctable_u->data ); free( ctable_u );
}


/* -----------------------------------------------
	storage cleanup
	----------------------------------------------- */
	
void lzh_encoder::cleanup_storage( void ) {
	if ( lz_store ) {
		for ( lz_code** lzp = lz_store; *lzp; lzp++ ) {
			if ( (*lzp)->data ) free ( (*lzp)->data );
			free( *lzp );
		}
		free( lz_store );
		lz_store = NULL;
	}
	if ( vli_store ) {
		for ( vli_code** vlip = vli_store; *vlip; vlip++ ) free ( *vlip );
		free( vli_store );
		vli_store = NULL;
	}
	if ( table_c ) cleanup_hcode( table_c ); table_c = NULL;
	if ( table_d ) cleanup_hcode( table_d ); table_d = NULL;
	if ( table_u ) cleanup_hcode( table_u ); table_u = NULL;
}


/* -----------------------------------------------
	constructor for lzh writer class
	----------------------------------------------- */
	
lzh_decoder::lzh_decoder( iostream* dest ) {
	// setup lz decoder, 
	lz_dec = new lz_decoder( dest );
	// preset storage
	conv_c = NULL;
	conv_d = NULL;
	conv_u = NULL;
}


/* -----------------------------------------------
	destructor for lzh writer class
	----------------------------------------------- */
	
lzh_decoder::~lzh_decoder( void ) {
	// cleanup storage, lz decoder
	cleanup_storage();
	delete ( lz_dec );
}


/* -----------------------------------------------
	lzh decode all symbols in current block
	----------------------------------------------- */
	
int lzh_decoder::decode_lzh( iostream* source ) {
	huffman_reader* h_dec;
	unsigned char num[4];
	unsigned char* cdata;
	int csize = 0;
	vli_code* vli;
	lz_code* lz;
	
	
	// setup huffman conversion tables
	setup_hconv( source ); // don't attempt with no data left!
	if ( ( !conv_c ) || ( !conv_d ) || ( !conv_u ) ) return -1;
	
	// read compressed size, LSB first
	source->read( (void*) num, 1, 4 );
	for ( int i = 0; i < 4; i++ )
		csize |= num[i] << ((3-i) * 8);
		
	// read compressed data, setup huffman decoder
	cdata = ( unsigned char* ) calloc( csize, 1 );
	if ( !cdata ) return -1;
	source->read( cdata, 1, csize );
	h_dec = new huffman_reader( cdata, csize );
	
	// calloc a little memory for lz and vli
	lz = ( lz_code* ) calloc( 1, sizeof( lz_code ) );
	lz->data = ( unsigned char* ) calloc( 256, 1 );
	vli = ( vli_code* ) calloc( 1, sizeof( vli_code ) );
	// no out of memory must occur here :-P
	
	// lz/vli/huffman decoding loop
	while ( true ) {
		lz->count = h_dec->decode_symbol( conv_c ); if ( !lz->count ) break;
		vli->len = h_dec->decode_symbol( conv_d );
		vli->res = h_dec->read_bits( vli->len );
		lz->dist = decode_vli( vli );
		if ( lz->dist == 0 ) for ( int i = 0; i < lz->count; i++ )
			lz->data[i] = h_dec->decode_symbol( conv_u );
		lz_dec->decode_lz( lz );
	}
	
	// cleanup
	free( cdata );
	free ( lz->data );
	free ( lz );
	free ( vli );
	cleanup_storage();
	
	
	return csize;
}


/* -----------------------------------------------
	setup huffman conversion tables
	----------------------------------------------- */
	
void lzh_decoder::setup_hconv( iostream* source ) {
	canonical_table* ctable_c;
	canonical_table* ctable_d;
	canonical_table* ctable_u;
	
	
	// read canonical tables
	ctable_c = read_ctable( source );
	ctable_d = read_ctable( source );
	ctable_u = read_ctable( source );
	// dangerous in case of out of memory (!!!)
	// also, don't attempt this with no data left in file
	
	// convert to hcode tables
	conv_c = convert_ctable2hconv( ctable_c );
	conv_d = convert_ctable2hconv( ctable_d );
	conv_u = convert_ctable2hconv( ctable_u );
	
	// memory cleanup
	free( ctable_c->data ); free( ctable_c );
	free( ctable_d->data ); free( ctable_d );
	free( ctable_u->data ); free( ctable_u );
}


/* -----------------------------------------------
	storage cleanup
	----------------------------------------------- */
	
void lzh_decoder::cleanup_storage( void ) {
	if ( conv_c ) cleanup_hconv( conv_c ); conv_c = NULL;
	if ( conv_d ) cleanup_hconv( conv_d ); conv_d = NULL;
	if ( conv_u ) cleanup_hconv( conv_u ); conv_u = NULL;
}
