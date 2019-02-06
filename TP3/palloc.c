
/**
 * @file palloc.c
 * @author François Cayre <francois.cayre@grenoble-inp.fr>
 * @date Tue Jan 23 12:11:29 2018
 * @brief Protected alloc.
 *
 * Protected Alloc.
 */

#include <stdio.h>    /* fprintf */
#include <stdlib.h>   /* size_t */
#include <sys/mman.h> /* mmap, munmap, mprotect */
#include <string.h>   /* memset, strerror */
#include <errno.h>    /* errno */
#include <unistd.h>   /* sysconf */
#include <signal.h>


#define PALLOC_CANARY    0xDEADBEEF


void *pmalloc( size_t size ) {

  size_t    canary = PALLOC_CANARY;
  size_t page_size = sysconf( _SC_PAGESIZE );
  size_t    npages = 0;
  size_t     total = 0;
  char        *ptr = NULL;

  /*
	ici on calcule la taille dont on a besoin en octet (bytes) 	
	4 pour stocker le nombre de page memoire que l'on utilise 
	+ 4 pour stocker le canari ( 0xDEEDBEEF )
	+ size la taille qua l'on veut actuellemnt allouer
	+ page_size , une page pour pouvoir creer une cloture electrique cf cours

	(il faut garder en tete que mmap (plus tard) alloue des block des pages entiere et donc size sera arrondi au nombre de page supérieur)
  */
  total = 8 + size + page_size;

  /*
     In mmap(2), 'total' will be internally rounded up to
     a multiple of 'page_size' (the kernel only allocates
     *whole* pages).
  */
  /*
	mmap demande à l'os d'allouer un certain nombre d octect (size)
	cf man mmap mais lui donner NULL en argument force mmap à retourner comme valeur
	le pointeur designant le debut de la premiere page allouée, on stocke ca momentanément dans ptr
  */
  ptr = (char*) mmap(NULL , total , PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,0,0);

  /* We also need the number of pages that were allocated. */
  while( npages*page_size < total ) npages++;

  /* TODO: Call mprotect(2) to make a dead page at the end. */
  /*
	On met en place la cloture electrique
	grace a mprtotect() on protege la derniere page allouée par mmap() contre l'ecriture et la lecture
	comme ca si on ecrit ou lit dans cette derniere page on provoque un segfault
  */
   mprotect(ptr +  (npages-1)*page_size ,page_size ,PROT_NONE);

  /* TODO: Compute value of 'ptr' returned to caller. */
   /*
		point un peu delicat :
		On veut coller la fin de notre bloc de données au debut de la page qui a été protégé contre l'ecriture et la lecture par mprotect précedemment 
		ainsi si on déborde de notre bloc de mémoire on provoque un segfault
		Ainsi on est donc obligé de laisser un vide au debut de notre premiere page
		On cacule et place dans ptr la vrai adrresse de depart du bloc de données 
		(qui est précedé du canari et du nombre de page)
		Finalement on retourne ptr
   */
  ptr = ptr + (page_size - (size%page_size));
  /* TODO: Place 'canary' and 'npages' where appropriate.
     Hint: Use memcpy(3).
   */
   memcpy(ptr -4,&canary,4);
   memcpy(ptr -8,&npages,4);

  return ptr;
}

//on senbalek
void *pcalloc( size_t nmemb, size_t size ) {
  size_t total = nmemb*size;
  void    *ptr = pmalloc( total );

  /* pcalloc as a wrapper around pmalloc + memset */
  memset( ptr, 0, total );

  return ptr;
}

/*
	Dans le free on  verifie :
	que le canari n'a pas été changé (on aurait corrompu la memoire)
	Puis on déalloue la mémoire en appelant munmap()
	cepandant il faut lui donner en parametre l'adresse de debut d'une page
	or nous on a que ptr qui est le debut du b loc de données
	Ainsi on a un petit calcul savant avec des cast de partout (notamment pour pouvoir faire des opérations sur les pointeur genre : rptr/page_size)
*/
void  pfree( void *ptr ) {
  char       *rptr = ptr; /* No pointer arithmetic on void* */
  size_t page_size = sysconf( _SC_PAGESIZE );
  size_t    canary = 0;
  size_t    npages = 0;

  /* TODO: Read 'canary' and 'npages' back.
     Hint: Use memcpy(3).
   */
   memcpy(&canary,rptr-4,4);
   memcpy(&npages,rptr-8,4);

  /* TODO: Check canary, possibly emit warning. */
  if(PALLOC_CANARY != canary){
    printf("data is corrupted !!!!\n" );
  }
  /* TODO: Call munmap(2) to actually free the mmap'd block. */
  munmap((void*)(page_size*(((size_t)rptr)/page_size)),npages*page_size);
  return;
}
