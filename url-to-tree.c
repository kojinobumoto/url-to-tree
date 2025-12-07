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

#define _POSIX_C_SOURCE 200809L
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
  size_t n_children;
};

void exitIfMemoryExhausted(const void *p, const char *s)
{
    if (p == NULL)
      {
        printf("Memory Exhausted for \"%s\" allocation.\n", s);
        exit(EXIT_FAILURE);
      }

    return;
}

int
countACharInString(const char *s, char c)
{
    int count = 0;
    while (*s) {
        if (*s == c) ++count;
        ++s;
    }
    return count;
}

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
    if (!p) return;
    size_t len = strlen(p);
    if (len == 0) return;
    if (p[len-1] == tc) p[len-1] = '\0';
}

void
tokenize( char **result, char *src, const char *delim, int token_size)
{
     int i=0;
     char *p=src;

     // limit this for loop at most till token_size time,
     // otherwise this loop might overwrite unwilling memory area.
     // (e.g. function variables in stack).
     for(result[i]=NULL, p=removeHeadingSubstr(p, delim); p!=NULL && *p && i < token_size; p=removeHeadingSubstr(p, delim) )
     {
         result[i++]=p;
         result[i]=NULL;
         p=strstr(p, delim);
     }
}

struct Node *
find_same_child(struct Node *p, const char *val)
{
    if (!p || !val || !p->children)
        return NULL;

    size_t n = p->n_children;

    for (size_t i = 0; i < n; ++i) {
        struct Node *child = p->children[i];
        if (child && strcmp(child->str, val) == 0)
            return child;
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

  int n_cldrn = nd->n_children;

  if (n_cldrn > 1)
    {
      qsort(nd->children, n_cldrn, sizeof(struct Node *), cmpNodeStr);
    }
    
  for(int i=0; i < n_cldrn; ++i)
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
  int n_cldrn = nd->n_children;
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

  for(int i=0; i < n_cldrn; ++i)
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

void freeNodes(struct Node *root)
{
    if (!root) return;

    size_t stack_cap = 1024;
    size_t stack_top = 0;
    struct Node **stack = malloc(stack_cap * sizeof(struct Node *));
    exitIfMemoryExhausted(stack, "stack");

    stack[stack_top++] = root;

    while (stack_top > 0)
    {
        /* pop */
        struct Node *nd = stack[--stack_top];

        for (size_t i = 0; i < nd->n_children; ++i) {
            if (stack_top >= stack_cap) {
                /* expand stack if necessary */
                stack_cap *= 2;
                struct Node **new_stack =
                    realloc(stack, stack_cap * sizeof(struct Node *));
                exitIfMemoryExhausted(new_stack, "new_stack");
                stack = new_stack;
            }
            stack[stack_top++] = nd->children[i];
        }

        /* free myself */
        free(nd->str);
        free(nd->title);
        free(nd->children);
        free(nd);
    }

    free(stack);
}

void
makeTree(FILE *stream, const char *delimiter, int flg_tsv_output)
{
  char *line = NULL;
  size_t len = 0;
  ssize_t nread;
  
  struct Node *nd = (struct Node *)calloc(1, sizeof(struct Node));
  exitIfMemoryExhausted(nd, "Node *nd");

  struct Node *root = nd;

  // create first child and starts from that child
  // and make sure there's always (num_of_children+1) children and let the last one is always null
  nd->children = NULL;

  while ((nread = getline(&line, &len, stream)) != -1)
    {
        
      size_t pos = 0;
      size_t url_len = 0;
      int flg_after_question_char = 0;
      char *url = NULL;
      char *title = NULL;
      char *dest_title = NULL;

      int token_size = countACharInString(line, '/');
      char * token[token_size + 1];
      
      nd = root;

      // '\r\n' should have been removed before tokenize.
      removeTrailingChar(line, '\n');
      removeTrailingChar(line, '\r');

      tokenize(token, line, delimiter, token_size + 1);
      
      // i'm interested in just url and title.
      url = token[0];
      if (token[1] != NULL)
        {
          int tl;
          title = token[1];
          tl = strlen(title);

          if(tl > 1 && title[tl-1] == '"' && title[tl-2] != '\\')
            {
              removeTrailingChar(title, '"');
            }

        }
      
      // ignore heading '"'
      if (*url == '"') { ++url; }
      if (title != NULL && *title == '"') { ++title; }
      
      url_len = strlen(url);
      
      while(pos != url_len && !isEndOfString(url[pos]))
        {

          int prev_pos = pos;
          char *dest = NULL;

          struct Node *child = NULL;

          // move after '/'
          if (url[pos] == '/')
            {
              url[pos] = '\0';
              ++pos;
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
              ++pos;
            }
          // Treat "://" to be one block.
          // Ttypically, move after "http://",
          // or we can ignore the "http(s)://" in the parameter
          // such as "twitter.com/?link=https://aaa.bbb.com".
          // (but this block might be useless for the latter case because of above while() block).
          if (pos > 0
              && (size_t)pos + 1 < url_len
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
                  ++pos;
                }
            }

          // now url[pos] is '/'
          url[pos] = '\0';
          
          child = find_same_child(nd, url+prev_pos);
          
          if (child == NULL)
            {
              dest = (char *)calloc(pos-prev_pos+1, sizeof(char));
              exitIfMemoryExhausted(dest, "dest");
              
              memmove(dest, &url[prev_pos], pos-prev_pos);

              size_t idx = nd->n_children;

              nd->children = realloc(nd->children, (idx + 1) * sizeof(struct Node*));
              exitIfMemoryExhausted(nd->children, "nd->children");

              // new child node
              child = calloc(1, sizeof(struct Node));
              exitIfMemoryExhausted(child, "new child");

              child->str = dest;
              child->children = NULL;
              child->n_children = 0;

              nd->children[idx] = child;
              nd->n_children++;
            }          

          nd = child;
          ++pos; // move to next char

        }
        // end of inner while

      if (title != NULL)
        {
          //trim unnecessary trailing '\r' or '\n' if exists.
          title[strcspn(title, "\r\n")] = '\0';
          if (strlen(title) > 0 )
            {
              dest_title = (char *) calloc(strlen(title)+1, sizeof(char));
              exitIfMemoryExhausted(dest_title, "dest_title");

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
