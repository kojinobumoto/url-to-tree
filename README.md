# url-to-tree

## DESCRIPTION
Convert url-list to tree structure.

### NOTES

If you contain title of the Web page in url-list, 
the url must be surounded by double quotes (at least '",' must present at the end of the url string),
and the url and title must be separeted by comma (,).

## Usage
Compile and run with list of url file.
```
gcc -Wall -g -o url-to-tree url-to-tree.c
./url-to-tree <utl-list-text-file>
```
Example
```
$ ./url-to-tree urls.csv
(root)
      +--- https://www.a.b.com
            |--- aaa		aaa-title-aaa-1    
                  |--- xxx		aaa-xxx-title5
                  +--- zzz		aaa-zzz-title4
            +--- bbb		bbb-title2    
                  +--- yyy		bbb-yyy-title3
```
here, urls.csv looks like...
```
"https://www.a.b.com/aaa",aaa-title-aaa-1
"https://www.a.b.com/bbb",bbb-title2
"https://www.a.b.com/bbb/yyy",bbb-yyy-title3
"https://www.a.b.com/aaa/zzz",aaa-zzz-title4
"https://www.a.b.com/aaa/xxx",aaa-xxx-title5
```
If you put "-tsv" option, the tree is printed out separated by TAB(s),
so that you can save and import the output to Excel or Google Sheet as a TSV file.
```
$ ./url-to-tree -tsv urls.csv
(root)
	https://www.a.b.com
aaa-title-aaa-1    		aaa
aaa-xxx-title5			xxx
aaa-zzz-title4			zzz
bbb-title2    		bbb
bbb-yyy-title3			yyy
```

## License
[MIT](https://choosealicense.com/licenses/mit/)
