/* -----------------------------------------------
	VLI struct
	----------------------------------------------- */
	
struct vli_code {
	int len;
	unsigned int res;
};


/* -----------------------------------------------
	VLI en/decoder functions
	----------------------------------------------- */
	
vli_code* encode_vli( int val );
int decode_vli( vli_code* vli );
