struct vc_data {
	unsigned long	space;
	unsigned char   vc_palette[16*3];        
};

struct vc {
	struct vc_data *d;
};

struct vc_data a_con;
struct vc vc_cons[63] = { &a_con };
int def_red[16];
int def_grn[16];
int def_blu[16];

extern void bar(int);

void reset_palette(int currcons)
{
	int j, k;
	for (j=k=0; j<16; j++) {
		(vc_cons[currcons].d->vc_palette) [k++] = def_red[j];
		(vc_cons[currcons].d->vc_palette) [k++] = def_grn[j];
		(vc_cons[currcons].d->vc_palette) [k++] = def_blu[j];
	}
	bar(k);
}

void bar(int k)
{
	if (k != 16*3)
		abort();
}

int main()
{
	reset_palette(0);
	exit(0);
}
