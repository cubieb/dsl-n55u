#ifdef TCSUPPORT_GENERAL_MULTILANGUAGE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parsers.h"
#include "../../../lib/libtcapi.h"
/*********internal variables************/
static	int	hashsize[] = {2531, 2743, 3137, 3529};//array of prime numbers
static	int	m;
static	SParser	s_parse;
static	char	str_attrname[32] = {0};
static	int	init_flag = 0;
static	FILE	*fpl = NULL;//global file pointer
/************internal method************/
static	uint	hash(uint k);
static	void	collision(uint *p, uint d);
static	void	reCreateHashTable(SParser *H);
static	void	ResetHashTable(SParser *H);
static	uint	getkey(const char *ckey);
static	int	Find(SParser *H, uint key, uint *p, uint *c);
static	int	insertToHash(SParser *H, HASH elem);
/************external method as below**********/
int		getString(const char *pkey, char *value);//get string from key character.
int		initandparserfile(void);//parse string file to create hash table
void		initParseStruct(void);//initializtion Parser struct;
void		closefp(void);//close string file pointer
int		islangChanged(void);//check the language is changed?


static	uint	hash(uint k){
	return k%m;
}

static	void	collision(uint *p, uint d){
	*p = (*p + d)%m;
}

static	int	initHashTable(SParser *H)
{
	int i=0;
	(*H).count = 0;
	(*H).sizeindex = 0;
	m = hashsize[(*H).sizeindex];
	(*H).ele = (HASH *)malloc(m*sizeof(HASH));
	if(!(*H).ele){
		return FAILURE;
	}
	for (i=0; i<m; i++){
		(*H).ele[i].key = NULLKEY;
		(*H).ele[i].ioffset = NULLKEY;
	}
	return SUCCESS;
}
static	void	reCreateHashTable(SParser *H)
{
	printf("need recreate memory\r\n");
	HASH	*p = NULL;
	HASH	*pold = (HASH *)malloc(m*sizeof(HASH));
	int	i = 0;
	int	count = (*H).count;
	
	if (!pold) 
		return;
	p = pold;
	for (; i<m; i++)
	{
		if((*H).ele[i].key != NULLKEY)
			*p++ = (*H).ele[i];
	}
	(*H).sizeindex++;
	(*H).count = 0;
	m = hashsize[(*H).sizeindex];
	p = (HASH *)realloc((*H).ele, m*sizeof(HASH));
	if (p == NULL)
	{
		tcdbg_printf("realloc memory failure\r\n");
		free(pold);
		return;
	}
	(*H).ele = p;
	for (i=0; i<m; i++)
	{
		(*H).ele[i].key = NULLKEY;
		(*H).ele[i].ioffset = NULLKEY;
	}
	
	for(p=pold; p<(pold+count); p++)
	{
		insertToHash(H, *p);
	}
	free(pold);
}
static	void	ResetHashTable(SParser *H)
{
	if (NULL != (*H).ele)
		free((*H).ele);
	(*H).ele = NULL;
	(*H).count = 0;
	(*H).sizeindex = 0;
}
static	uint	getkey(const char *ckey)
{
	uint	sum = 0;
	uint	seek = 131;
	
	while(*ckey){
		sum = sum*seek + (*ckey++);
	}
	return (sum & 0X7FFFFFFFL);
	
}
static	int	Find(SParser *H, uint key, uint *p, uint *c)
{
	*p = hash(key);
	while(((*H).ele[*p].key != NULLKEY) && ((*H).ele[*p].key != key))
	{
		(*c)++;
		if ((*c)<m)
			collision(p, *c);
		else
			return FAILURE;
	}
	if ((*H).ele[*p].key == key)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

static	int	insertToHash(SParser *H, HASH elem)
{
	int p,c=0;
	
	if (SUCCESS == Find(H, elem.key, &p, &c))
	{
		tcdbg_printf("%s duplicate, please check!\r\n",str_attrname);
		return DUPLICATE;
	}
	else if(c < (m/2)){
		(*H).ele[p] = elem;
		(*H).count++;
		return SUCCESS;
	}
	else{
		//wait to recreatehash table;
		reCreateHashTable(H);
		return FAILURE;
	}
}

//interface functions as below
int	getString(const char *pkey, char *value)
{
	if ((0 == strlen(pkey)) || (NULL == fpl))	return FAILURE;
	uint p, key, c = 0;
	key = getkey(pkey);
	Find(&s_parse, key, &p, &c);
	uint offset = s_parse.ele[p].ioffset;
	
	//FILE	*fp = NULL;
	char	*pk = NULL;
	char	stream[MAX_STREAM] = {0};
	char	str_value[MAX_VALUE] = {0};
	int	totalLength, enterOffSet = 0;
	
	/*fp=fopen("string1.conf","r");
	if(fp==NULL)
	{
		printf("\r\nCan't open string1.conf\n");
		return NULL;
	}*/
	
	fseek(fpl, offset, SEEK_SET);
	while(fgets(stream, MAX_STREAM, fpl) != NULL){
	
		enterOffSet = 0;
		if(strrchr(stream,'\r') != NULL)
		{
			enterOffSet++;
		}
		if(strrchr(stream,'\n') != NULL)
		{
			enterOffSet++;
		}

		pk = strstr(stream, pkey);
		
		if ((pk != NULL) && (pk == stream))
		{
			totalLength=strlen(stream)-strlen(pkey)-1-enterOffSet;//1 means '=' length
			if(MAX_VALUE<totalLength+1) 		/*total length + '\0' should not less than buffer*/
			{
				//tcdbg_printf("\r\nToo small buffer to catch the %s frome get_string_value\n", keyname);
				//fclose(fp);
				return FAILURE;
			}
			else if(totalLength<0)		/*can't get a negative length string*/ 
			{
				//printf("\r\nNo profile string can get\n");
				//fclose(fp);
				return FAILURE;
			}
			else
			{
				strncpy(str_value, pk+strlen(pkey)+1, totalLength);
				str_value[totalLength] = '\0';
				strcpy(value, str_value);
				//if (0 != flag){
				//	tcdbg_printf("\r\n%s:Can't get string value from hash table\n", pkey);
				//}
				//fclose(fp);
				return strlen(value);
			}
			
		}
		else{
			break;
		}
	}
	//fclose(fp);
	tcdbg_printf("\r\n%s:Can't get string value from file\n", pkey);
	return FAILURE;
	
}

/* Paul modify 2013/2/7 */
int	initandparserfile(void)
{
	//FILE *fp = NULL;
	char *pk = NULL;
	char	str_path[PATH_LENGTH] = {0};
	char stream[MAX_STREAM] = {0};
	char str_key[MAX_KEY] = {0};
	char	str_type[4] = {0};
	int	nIndex = 0;
	uint	offset = 0;
	HASH	s_hash;
	SParser	s_parse_tmp;

	//Andy Chiu, 2015/03/03. retry for cfg_manager restart scoket issue.
	int res = 0, i;
	for(i = 0; i < 5; ++i)
	{
		res = tcapi_get("LanguageSwitch_Entry", "Type", str_type);
		if(!res && strlen(str_type) > 0 )
			break;
		
		tcdbg_printf("[%s, %d]get lang failed!\n", __FUNCTION__, __LINE__);
		sleep(1);
	}
	
	if (strlen(str_type))
		nIndex = atoi(str_type);

	init_flag = nIndex;
	closefp();//close file pointer at first
	
	if(nIndex == 1) //English
		sprintf(str_path, STRING_PATH, "EN");
	else if(nIndex == 2) //Brazil
		sprintf(str_path, STRING_PATH, "BR");
	else if(nIndex == 3) //Simplified Chinese
		sprintf(str_path, STRING_PATH, "CN");
	else if(nIndex == 4) //Cesky
		sprintf(str_path, STRING_PATH, "CZ");
	else if(nIndex == 5) //Dansk
		sprintf(str_path, STRING_PATH, "DA");
	else if(nIndex == 6) //Deutsch
		sprintf(str_path, STRING_PATH, "DE");
	else if(nIndex == 7) //Espanol
		sprintf(str_path, STRING_PATH, "ES");
	else if(nIndex == 8) //Finsk
		sprintf(str_path, STRING_PATH, "FI");
	else if(nIndex == 9) //Francais
		sprintf(str_path, STRING_PATH, "FR");
	else if(nIndex == 10) //Italiano
		sprintf(str_path, STRING_PATH, "IT");
	else if(nIndex == 11) //Malay
		sprintf(str_path, STRING_PATH, "MS");
	else if(nIndex == 12) //Norsk
		sprintf(str_path, STRING_PATH, "NO");
	else if(nIndex == 13) //Polski
		sprintf(str_path, STRING_PATH, "PL");
	else if(nIndex == 14) //Russian
		sprintf(str_path, STRING_PATH, "RU");
	else if(nIndex == 15) //Svensk
		sprintf(str_path, STRING_PATH, "SV");
	else if(nIndex == 16) //Thai
		sprintf(str_path, STRING_PATH, "TH");
	else if(nIndex == 17) //Turkey
		sprintf(str_path, STRING_PATH, "TR");
	else if(nIndex == 18) //Traditional Chinese
		sprintf(str_path, STRING_PATH, "TW");
	else if(nIndex == 19) //Ukraine
		sprintf(str_path, STRING_PATH, "UK");
	else
		sprintf(str_path, STRING_PATH, "EN");
		
	fpl=fopen(str_path,"r");
	if(fpl == NULL)
	{
		tcdbg_printf("\r\n%s:can't open %s\r\n",__FUNCTION__, str_path);
		return FAILURE;
	}
	
	if (FAILURE == initHashTable(&s_parse_tmp)) {
		tcdbg_printf("\r\n%s:alloc memory\n", __FUNCTION__);
		return FAILURE;
	}
	ResetHashTable(&s_parse);//free memory at first
	s_parse = s_parse_tmp;
	
	fseek(fpl, 0, SEEK_SET);
	while(fgets(stream, MAX_STREAM, fpl) != NULL)
	{
		int	len = strlen(stream);
		pk = strstr(stream,"=");
		if ((stream[0] == ';') || (NULL == pk)) {
			offset += len;
			continue;
		}
		
		*pk = '\0';
		
		strncpy(str_key, stream, strlen(stream));
		s_hash.key = getkey(str_key);
		s_hash.ioffset = offset;
		offset += len;
		
		strcpy(str_attrname, str_key);
		insertToHash(&s_parse, s_hash);
		
		memset(stream, 0, MAX_STREAM);
		memset(str_key, 0, MAX_KEY);
	}
	//fclose(fp);
	return SUCCESS;
}
void	closefp(void)
{
	if (NULL != fpl){
		fclose(fpl);
		fpl = NULL;
	}
}
void	initParseStruct(void)
{
	s_parse.ele = NULL;
	s_parse.count = 0;
	s_parse.sizeindex = 0;
}
int	islangChanged(void)
{
	char	str_type[4] = {0};
	tcapi_get("LanguageSwitch_Entry", "Type", str_type);
	
	//tcdbg_printf("\r\n*** islangChanged - atoi(str_type): [%d]\n", atoi(str_type));
	if (atoi(str_type) != init_flag)
	{
		return 1;
	}

	return 0;
}
#endif
