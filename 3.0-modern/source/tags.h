/* SCCS ID: tags.h 1.1 8/5/93 */
int LoadTagsFile(char *filename);
int LookupTag(char *name, char **file, char **searchString);
int FindDefinition(WindowInfo *window, Time time);
int TagsFileLoaded();
