#ifdef __pdp10__
f(got){if(got!=0777777)abort();}
#else
f(got){if(got!=0xffff)abort();}
#endif
main(){signed char c=-1;unsigned u=(unsigned short)c;f(u);exit(0);}
