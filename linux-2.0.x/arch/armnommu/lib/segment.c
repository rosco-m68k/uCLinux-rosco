memcpy(char *to, char *from, int count) {
if (to < 0x200000) {
    printk("mcpy %p %p size %d\n",to,from,count);
  }
  while(count--) *(to++) = *(from++); 
}
memcpy_tofs(int x, int y, int z) {return memcpy(x,y,z);}
memcpy_fromfs(int x, int y, int z) {return memcpy(x,y,z);}
memmove(int x, int y, int z) {return memcpy(x,y,z);}
__memcpy_fromfs(int x, int y, int z) {return memcpy(x,y,z);}
__memcpy_tofs(int x, int y, int z) {return memcpy(x,y,z);}
