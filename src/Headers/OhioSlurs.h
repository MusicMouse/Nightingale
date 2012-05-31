enum {
		V_Single,
		V_Upper,
		V_Middle,
		V_Lower
	};

enum {
		N_First,
		N_Intermediate,
		N_Last
	};

/* Prototypes for public routines in library */

void	SL_InitSlur(DDIST dy,int voice);
void	SL_DeclareNote(int type, int stem, int acc, int beam, DDIST x, DDIST y, DDIST stlen);
void	SL_GetSlur(DPoint *start, DPoint *c0, DPoint *c1, DPoint *end);
