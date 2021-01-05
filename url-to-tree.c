/*
 * FILENAME :    url-to-tree.c
 *
 * DESCRIPTION : 
 *         Convert url-list to tree structure.
 *
 * NOTES :
 *         If you contain title of the Web page in url-list, 
 *         the url must be surounded by double quotes (at least '",' must present at the end of the url string),
 *         and the url and title must be separeted by comma (,).
 *
 *             e.g.
 *             "https://www.a.b.com/aaa",title1
 *             "https://www.a.b.com/bbb",title2
 *             "https://www.a.b.com/bbb/yyy",title3
 *             "https://www.a.b.com/aaa/zzz",title4
 *             "https://www.a.b.com/aaa/xxx",title5
 *             
 *             ....goes to
 *             
 *             https://www.a.b.com
 *                  |--- aaa     title1
 *                        |--- xxx    title5
 *                        +--- zzz    title4
 *                  |--- bbb     title2
 *                        +--- yyy    title3
 *
 *         If you put "-tsv" option, the tree is printed out separated by TAB(s),
 *         so that you can import the output to Excel or Google Sheet as a TSV file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef DEBUG
#define DEBUG 0
#endif

//typedef struct Node Node;
struct Node {
  char *str;
  char *title;
  //struct Node *parent;
  struct Node **children;
};

/*
void
removeSubstr (char *string, char *sub) {
    char *match;
    int len = strlen(sub);
    while ((match = strstr(string, sub))) {
      memset(match, '\0', len);
    }
}
*/

char *
removeHeadingSubstr(char *p, const char *sub)
{
  size_t len=strlen(sub);     
  while(p!=NULL && *p && memcmp(p, sub, len)==0)
    {
      memset(p, '\0', len);
      p+=len;    
    }
  return p;
}

void
removeTrailingChar(char *p, const char tc)
{
  size_t len=strlen(p);
  if (p[len-1] == tc)
    {
      p[len-1] = '\0';
    }
  return;
}

void
tokenize( char **result, char *src, const char *delim)
{
     int i=0;
     char *p=NULL;
     p=src;
     for(result[i]=NULL, p=removeHeadingSubstr(p, delim); p!=NULL && *p; p=removeHeadingSubstr(p, delim) )
     {
         result[i++]=p;
         result[i]=NULL;
         p=strstr(p, delim);
     }
}

int
count_children(struct Node *p)
{
  if (p->children == NULL)
    {
      return 0;
    }

  int i = 0;
  while(p->children[i] != NULL)
    {
      i++;
    }

  return i;

}

struct Node*
find_same_child(struct Node *p, char *val)
{
  int n_cldrn = 0;

  // if no children
  if (p->children == NULL)
    {
      return NULL;
    }

  n_cldrn = count_children(p);

  for (int i = 0; i < n_cldrn; i++)
    {
      if(p->children[i] == NULL)
        {
          return NULL;
        }
      if(strcmp(p->children[i]->str, val) == 0)
        {
          return p->children[i];
        }
    }
  return NULL;
}

int
cmpNodeStr(const void * a, const void * b)
{
  const struct Node *n1 = *(const struct Node **)a;
  const struct Node *n2 = *(const struct Node **)b;

  return strcmp(n1->str, n2->str);

}

void
sortChildren(struct Node *nd)
{

  int n_cldrn = count_children(nd);

  if (n_cldrn > 1)
    {
      qsort(nd->children, n_cldrn, sizeof(struct Node *), cmpNodeStr);
    }
    
  for(int i=0; i < n_cldrn; i++)
    {
      sortChildren(nd->children[i]);
    }
    
}

int
isEndOfString(char c)
{
  return (c == '\0' || c == '\r' || c == '\n');
}

void
indentWithChar(int n, char c) {
  for(int i = 0; i < n; i++)
    {
      putchar(c);
    }
}

enum printTypes{ PLANE, TSV };

void
printTree(struct Node *nd, int depth, int flg_lastchild, int print_type)
{
  int n_cldrn = count_children(nd);
  int flg_lc = flg_lastchild;

  if (nd->str == NULL)
    {
      printf("(root)\n");
    }
  else
    {
      switch(print_type)
        {
        case PLANE:
        default:
          // print some indent
          printf("%*s", depth*6, "");
          if(flg_lc == 0)
            {
              if (nd->title == NULL)
                {
                  printf("|--- %s\n", nd->str);
                }
              else
                {
                  printf("|--- %s\t\t%s\n", nd->str, nd->title);
                }
            }
          else
            {
              if (nd->title == NULL)
                {
                  printf("+--- %s\n", nd->str);
                }
              else
                {
                  printf("+--- %s\t\t%s\n", nd->str, nd->title);
                }
            }
          break;
        case TSV:
          if (nd->title == NULL)
            {
              indentWithChar(depth, '\t');
              printf("%s\n", nd->str);
            }
          else
            {
              printf("%s", nd->title);
              indentWithChar(depth, '\t');
              printf("%s\n", nd->str);
            }
          break;
        }
    }

  depth++;

  for(int i=0; i < n_cldrn; i++)
    {
      flg_lc = 0;
      if(i == n_cldrn - 1)
        {
          flg_lc = 1;
        }
      printTree(nd->children[i], depth, flg_lc, print_type);
    }
  return;
}

void
freeNodes(struct Node *nd)
{
  int n_cldrn = 0;
  if (nd != NULL)
    {
      n_cldrn = count_children(nd);
      for (int i=0; i < n_cldrn; i++) {
        freeNodes(nd->children[i]);
      }
      if (nd->str != NULL) { free(nd->str); }
      if (nd->title != NULL) { free(nd->title); }
      if (nd->children != NULL) { free(nd->children); }
      free(nd);
      
    }
}

void
makeTree(FILE *stream, const char *delimiter, int flg_tsv_output)
{
  char *line = NULL;
  size_t len = 0;
  ssize_t nread;
  int n_cldrn = 0;
  char * token[10];
  
  struct Node *nd = (struct Node *)calloc(1, sizeof(struct Node));
  if (nd == NULL)
    {
      printf("Memory Exhausted for \"Node *nd\" allocation.\n");
      exit(EXIT_FAILURE);
    }
  struct Node *root = nd;

  // create first child and starts from that child
  // and make sure there's always (n_cldrn+1) children and let the last one is always null
  nd->children = NULL;

  while ((nread = getline(&line, &len, stream)) != -1)
    {
        
      int pos = 0;
      int url_len = 0;
      int flg_after_question_char = 0;
      char *url = NULL;
      char *title = NULL;
      char *dest_title = NULL;
      
      nd = root;

      removeTrailingChar(line, '\n');
      removeTrailingChar(line, '\r');
      
      tokenize(token, line, delimiter);
      
      // i'm interested in just url and title.
      url = token[0];
      if (token[1] != NULL)
        {
          int tl;
          title = token[1];
          
          tl = strlen(title);
          /*// '\r\n' should have been removed before tokenize.
          if(title[tl-1] == '\n') {removeTrailingChar(title, '\n'); }
          tl = strlen(title);
          if(title[tl-1] == '\r') {removeTrailingChar(title, '\r'); }
          tl = strlen(title);
          */
          if(tl > 1 && title[tl-1] == '"' && title[tl-2] != '\\')
            {
              removeTrailingChar(title, '"');
            }

        }
      
      // ignore heading '"'
      if (*url == '"') { url++; }
      if (title != NULL && *title == '"') { title++; }
      
      url_len = strlen(url);
      
      while(pos != url_len && !isEndOfString(url[pos]))
        {

          int prev_pos = pos;
          char *dest = NULL;

          struct Node *child = NULL;

          n_cldrn = 0;

          // move after '/'
          if (url[pos] == '/')
            {
              pos++;
              prev_pos = pos;
            }
          if (isEndOfString(url[pos]))
            {
              break;
            }
          // count character between '/' and '/'.
          // If the '?' character was found, the string after '?' is just a parameter
          // so that ignore any occurrences of '/' after '?'.
          while((flg_after_question_char || url[pos] != '/') && !isEndOfString(url[pos]))
            {
              if (url[pos] == '?')
                {
                  flg_after_question_char = 1;
                }
              pos++;
            }
          // Treat "://" to be one block.
          // Ttypically, move after "http://",
          // or we can ignore the "http(s)://" in the parameter
          // such as "twitter.com/?link=https://aaa.bbb.com".
          // (but this block might be useless for the latter case because of above while() block).
          if (pos > 0
              && url[pos-1] == ':'
              && url[pos+1] == '/')
            {
              //does not need to modify prev_pos at here.
              pos+=2;
              // again count character between '/' and '/'
              while((flg_after_question_char || url[pos] != '/') && !isEndOfString(url[pos]))
                {
                  if (url[pos] == '?')
                    {
                      flg_after_question_char = 1;
                    }
                  pos++;
                }
            }

          dest = (char *)calloc(pos-prev_pos+1, sizeof(char));
          if (dest == NULL)
            {
              printf("Memory Exhausted for \"dest\" allocation.\n");
              exit(EXIT_FAILURE);
            }
          memmove(dest, &url[prev_pos], pos-prev_pos);
          n_cldrn = count_children(nd);
          child = find_same_child(nd, dest);

          if (child == NULL)
            {
              // make sure there's always (n_cldrn+1) children and let the last one is always null
              nd->children = (struct Node **)realloc(nd->children, (n_cldrn+2)*sizeof(struct Node*));
              if (nd->children == NULL)
                {
                  printf("Memory Exhausted for \"nd->children\" allocation.\n");
                  exit(EXIT_FAILURE);
                }

              child = (struct Node *)calloc(1, sizeof(struct Node));
              child->str = dest;
              child->children = NULL;

              nd->children[n_cldrn] = child;
              nd->children[n_cldrn+1] = NULL;
            }
          else
            {
              // same child exists, dest is useless at this moment.
              free(dest);
            }

          nd = child;

        }
        // end of inner while

      if (title != NULL)
        {
          //trim unnecessary trailing '\r' or '\n' if exists.
          title[strcspn(title, "\r\n")] = '\0';
          if (strlen(title) > 0 )
            {
              dest_title = (char *) calloc(strlen(title)+1, sizeof(char));
              if (dest_title == NULL)
                {
                  printf("Memory Exhausted for \"dest_title\" allocation.\n");
                  exit(EXIT_FAILURE);
                }
              memmove(dest_title, title, strlen(title));
              nd->title = dest_title;
            }

        }

    }

  sortChildren(root);
  
  //print out
  printTree(root, 0, 0, flg_tsv_output);

  freeNodes(root);
  free(line);
  return;
}

void
printUsage(char * myname)
{
  fprintf(stderr, "Usage is %s [options] <file>\n", myname);
  fprintf(stderr, "Option(s)\n");
  fprintf(stderr, "\t-tsv\teach coulmn(s) are separated by TAB. Title will be the first column.\n");
  fprintf(stderr, "\t-h\tPrint this help.\n");
  exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
  FILE *stream;
  char *myname; // this program's name

  const char *delimiter = "\",";
  
  int flg_TSVoutput = 0;
  
  myname = argv[0];

  if (argc < 2) {
    printUsage(myname);
  }

  // handle arguments.
  while( (argc >= 2) && (argv[1][0] == '-'))
    {
      if (strcmp(argv[1], "-tsv") == 0)
        {
          flg_TSVoutput = 1;
        }
      else if (strcmp(argv[1], "-h") == 0)
      {
        printUsage(myname);
      }
      else
        {
          fprintf(stderr, "Bad option %s\n", argv[1]);
          printUsage(myname);
        }
      ++argv;
      --argc;
    }
  
  stream = fopen(argv[1], "r");
  if (stream == NULL)
    {
      perror("fopen");
      exit(EXIT_FAILURE);
    }
  
  makeTree(stream, delimiter, flg_TSVoutput);

  fclose(stream);
  exit(EXIT_SUCCESS);
}
