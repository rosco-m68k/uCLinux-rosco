/*
 *   FILE: abs.c
 * AUTHOR: kma
 *  DESCR: absolute value; we need this
 */

#ident "$Id: abs.c,v 1.1 1999/12/03 06:02:34 gerg Exp $"

int abs(int j)
{
	return (j<0)? -j : j;
}
