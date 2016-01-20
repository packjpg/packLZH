#define LZ_D_LMAX	( 3 + 10 + 10 ) // log max of LZ distance

/* -----------------------------------------------
	lz coder classes
	----------------------------------------------- */

	
struct lz_code {
	int count;
	int dist;
	unsigned char* data;
};

struct htab_entry {
	int pos;
	htab_entry* next; 
};

class lz_encoder {
	public:
	lz_encoder( iostream* source );
	lz_encoder( iostream* source, int lim_count, int lim_dist );
	~lz_encoder( void );
	// encoding function
	lz_code* encode_lz( void );
	
	private:
	// encoding helper functions
	int find_best_match( lz_code* lz );
	int current_byte( unsigned char* byte );
	void advance_stream( int n );
	// utility functions
	inline unsigned char stream( unsigned int apos );
	inline int gen_hash( void );
	inline void clear_ht( void );
	inline void clear_ht_entry( htab_entry** start );
	
	// storage
	lz_code* lz_next;
	htab_entry** hash_table;
	unsigned char* lz_buffer;
	iostream* str_src;
	int fpos;
	int lhash;
	int remain;
	int limc;
	int limd;
	int limcd;
};

class lz_decoder {
	public:
	lz_decoder( iostream* dest );
	~lz_decoder( void );
	// public functions
	void decode_lz( lz_code* lz );
	
	private:
	// decoding helper functions
	void write_lz_code( lz_code* lz );
	void write_nbyte( unsigned char* data, int n );
	// storage
	unsigned char* lz_buffer;
	iostream* str_dst;
	int fpos;
};
