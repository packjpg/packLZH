#include "lzcoder.h" // this is meant as an extension to the LZ coder ...
#include "huffcoder.h" // .. and huffman coder ...
#include "vlicoder.h" // ... also utilizing the vli coder


/* -----------------------------------------------
	lzh coder classes
	----------------------------------------------- */
	
class lzh_encoder {
	public:
	lzh_encoder( iostream* source );
	lzh_encoder( iostream* source, int lim_count, int lim_dist );
	~lzh_encoder( void );
	// encoding function
	int encode_lzh( iostream* dest, int nsymb );
	
	private:
	void setup_htables( iostream* dest );
	void cleanup_storage( void );
	
	lz_encoder* lz_enc;	
	lz_code** lz_store;
	vli_code** vli_store;
	
	huffman_code** table_c;
	huffman_code** table_d;
	huffman_code** table_u;
};

class lzh_decoder {
	public:
	lzh_decoder( iostream* dest );
	~lzh_decoder( void );
	// decoding functions
	int decode_lzh( iostream* source );
	
	private:
	void setup_hconv( iostream* source );
	void cleanup_storage( void );
	
	lz_decoder* lz_dec;
	
	huffman_conv_set* conv_c;
	huffman_conv_set* conv_d;
	huffman_conv_set* conv_u;
	
	iostream* str_dst;
};
