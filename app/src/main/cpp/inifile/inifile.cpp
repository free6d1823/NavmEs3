#include "inifile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class Node {
public:
    Node (const char* name){
        mName = strdup(name);
        mPrev=NULL;
        mNext=NULL;
        mChild=NULL;
        mParent = NULL;
    }
    ~Node(){
        if (mName) free(mName);
        if (mChild)
            killAllChildren();
        if (mPrev)
            mPrev->mNext = mNext;
        if(mNext)
            mNext->mPrev = mPrev;
        if (mParent)
            mParent->mChild = mNext;

    }
    char* name(){return mName;}
    Node* next(){return mNext;}
    Node* prev(){return mPrev;}
    Node* firstChild(){return mChild;}
    Node* lastChild(){
        if (!mChild)
            return mChild;
        Node* pNext = mChild;
        while(pNext->next())pNext=pNext->next();
        return pNext;
    }

    /*<! insert a sibling below this */
    Node* insert(Node* node){
        node->mPrev = this;
        node->mNext = this->next();
        mNext = node;
        if(node->mNext)node->mNext->mPrev = node;
        return node;
    }
    /*<! append a child in the end*/
    Node* append(Node* node){
        if(!mChild) {
            mChild = node;
            node->mParent = this;
        } else {
            Node* first = mChild;
            while (first->next()){
                first = first->next();
            }
            first->mNext = node;
            node->mPrev = first;
        }
        return node;
    }

    /*<! killAllChildren */
    void  killAllChildren(){
        if (!mChild)
            return;
        Node* pNext = mChild;
        while(pNext){
            Node* tmp = pNext;
            pNext = pNext->mNext;
            delete tmp;
        }
    }

    /*<! find a child with name */
    Node* findChild(const char* name) {
        Node* first = mChild;

        while(first) {
            if( strcmp(first->mName, name)==0){
                break;
            }
            first = first->next();
        }
        return first;
    }

public:
    Node* mPrev;
    Node* mNext;
    Node* mChild;
    Node* mParent;
    char* mName;
};
class Leave : public Node{
public:
    Leave(const char* name, const char* value):Node(name){

        mValue  = strdup(value);
    }
    ~Leave(){
        if (mValue) free(mValue);

    }
    char* value(){return mValue;}
    void setValue(const char* value){
        if (mValue) free (mValue);
        mValue = strdup(value);
    }

private:

    char* mValue;
};

typedef struct _INI_CB{
    char* ininame;
    bool readonly;
    Node* root;
}INI_CB;

static Node* findSection(char* buffer)
{
    char* p1 = strchr(buffer, '[');
    if(!p1)
        return NULL;
    p1++;
    char* p2 = strchr(p1, ']');
    if (!p2)
        return NULL;
    *p2 = 0;
    Node* node = new Node(p1);
    return node;
}
static Leave* parserLeave(char* buffer)
{
    char* p2 = strchr(buffer, '=');
    if (!p2)
        return NULL;
    *p2++ = 0;
    while(*p2 == ' ')p2++;
    char* p3;
    p3 = p2 + strlen(p2) -1;
    while(*p3 == '\r' || *p3 == '\n' || *p3 == ' '){
        *p3 = 0; p3--;
    }
    char* p1 = buffer;
    while(*p1 == ' ')p1++;
    p3 = p1 + strlen(p1)-1;
    while(*p3 == ' '){
        *p3 = 0; p3--;
    }
    Leave* leave = new Leave(p1,p2);
    return leave;
}
static bool writeSection(Node* pNode, FILE* fp)
{
    fprintf(fp, "[%s]\r\n", pNode->name());

    Leave* pLeave = (Leave*) pNode->firstChild();
    while(pLeave){
        fprintf(fp, "%s=%s\r\n", pLeave->name(), pLeave->value());
        pLeave = (Leave*) pLeave->next();
    }
    return true;
}

void* openIniFile(const char* iniFilename, bool readOnly)
{
    char* pIniName = strdup (iniFilename);
    FILE* fp;
    if(readOnly) fp = fopen(pIniName, "r");
    else fp = fopen(pIniName,"w+");
    if (!fp){
        fprintf(stderr, "open file %s error!\n", iniFilename);
        return NULL;
    }
    INI_CB* handle = (INI_CB*) malloc(sizeof(INI_CB));
    handle->readonly = readOnly;
    handle->ininame = pIniName;
    handle->root = new Node("root");
    //parser file
    char buffer[512];

    while (fgets(buffer, sizeof(buffer), fp)) {
        if(buffer[0]==';')
            continue;
        if(buffer[0] == '[') {
            Node* pNode = findSection(buffer);
            if (pNode)
                handle->root->append(pNode);
        } else {
            Node* pNode = handle->root->lastChild();
            if (pNode) {
            Leave* pLeave = parserLeave(buffer);
                if(pLeave)
                    pNode->append(pLeave);
            }
        }
    }
    //
    fclose(fp);
    return (void*) handle;
}

void closeIniFile(void* handle)
{
    INI_CB* pCb = (INI_CB* )handle;
    if (pCb->root &&pCb->ininame && !pCb->readonly){
        FILE* fp = fopen(pCb->ininame, "w");
        if (fp) {
            ///////////////////////////
            ///
            ///
            Node* pSection = pCb->root->firstChild();
            while (pSection){
                writeSection(pSection, fp);
                pSection = pSection->next();
            }
            fclose(fp);
        }

    }
    free(pCb->ininame);
    if(pCb->root) delete pCb->root;
    free(pCb);
}
bool saveAsIniFile(void* handle, const char* iniName)
{
    INI_CB* pCb = (INI_CB* )handle;
    FILE* fp = fopen(iniName, "w");
    if (fp) {
        ///////////////////////////
        ///
        ///
        Node* pSection = pCb->root->firstChild();
        while (pSection){
            writeSection(pSection, fp);
            pSection = pSection->next();
        }
        fclose(fp);
        return true;
    }
    return false;
}

bool GetProfileString( const char *section, const char *key,char *value, int size,const char *default_value, void * handle)
{
    INI_CB* pCb = (INI_CB* )handle;
    if (!pCb->root)
        return false;
    Node* pNode = pCb->root->findChild(section);
    if(pNode){
        Leave* pLeave = (Leave*) pNode->findChild(key);
        if (pLeave) {
            strncpy(value, pLeave->value(), size);
            return true;
        }
    }
    strncpy(value, default_value, size);
    return true;
}

bool WriteProfileString( const char *section, const char *key,const char *value, void *handle)
{
    INI_CB* pCb = (INI_CB* )handle;
    if (!pCb->root)
        return false;
    Node* pNode = pCb->root->findChild(section);
    if (!pNode) {
        pNode = new Node(section);
        pCb->root->append(pNode);
    }
    Leave* pLeave = (Leave*) pNode->findChild(key);
    if (!pLeave){
        pLeave = new Leave(key, value);
        pNode->append(pLeave);
    }else {
        pLeave->setValue(value);
    }
    return true;
}

int GetProfileInt( const char *section, const char *key,int default_value, void * handle)
{
    char buffer[256];
    sprintf(buffer, "%d", default_value);
    if (GetProfileString(section, key,buffer, sizeof(buffer),buffer, handle)){
        return atoi(buffer);
    }
    return default_value;
}

bool WriteProfileInt( const char * lpSecName,const char * lpKeyName, int value, void * handle)
{
    char buffer[64];
    sprintf(buffer, "%d", value);

    return WriteProfileString(lpSecName, lpKeyName, buffer, handle);
}


/*<! *************************     extented Profile functions      *************************************/
bool WriteProfileFloat(  const char *lpSecName, const char * lpKeyName, float value,  void * handle)
{
    char szString[32];
    sprintf(szString, "%10.5f", value);

    return WriteProfileString(lpSecName, lpKeyName, szString, handle);
}
bool WriteProfileDouble(  const char *lpSecName, const char * lpKeyName, double value,  void * handle)
{
    char szString[32];
    sprintf(szString, "%12.7f", value);

    return WriteProfileString(lpSecName, lpKeyName, szString, handle);
}
float GetProfileFloat(const char *lpSecName, const char *lpKeyName, float fDefault,  void * handle)
{
    char value[64];
    if( false == GetProfileString(lpSecName, lpKeyName, value, sizeof(value), "0", handle))
        return fDefault;
    return (float) atof(value);
}
double GetProfileDouble(const char *lpSecName, const char *lpKeyName, double dbDefault,  void * handle)
{
    char value[64];
    if( false == GetProfileString(lpSecName, lpKeyName, value, sizeof(value), "0", handle))
        return dbDefault;
    return atof(value);
}

bool WriteProfilePointFloat(const char *lpSecName, const char *lpKeyName,  nfFloat2D *pPoint, void * handle)
{
    char szString[64];
    sprintf(szString, "%10.5f, %10.5f", pPoint->x, pPoint->y);
    return WriteProfileString(lpSecName, lpKeyName, szString, handle);
}
bool GetProfilePointFloat(const char * lpSecName, const char * lpKeyName,  nfFloat2D* pPoint, void * handle)
{
    char value[256];
    if( false == GetProfileString(lpSecName, lpKeyName, value, sizeof(value), "0", handle))
        return false;

    char* p1 = strtok(value, ",");
    if(!p1) return -1;
    pPoint->x = (float) atof(p1);
    p1 = strtok(NULL,",;");
    if(!p1) return false;
    pPoint->y = (float) atof(p1);
    return true;
}

bool WriteProfileArrayFloat(const char * lpSecName, const char * lpKeyName,  float* pValue, int nElements, void * handle)
{
    char szString[64];
    char szArray[512]={0};
    for (int i=0; i< nElements; i++) {
        sprintf(szString, "%10.5f,", pValue[i]);
        strcat(szArray, szString);
    }
    return WriteProfileString(lpSecName, lpKeyName, szArray, handle);
}
bool GetProfileArrayFloat(const char * lpSecName, const char * lpKeyName,   float* pValue, int nElements,  void * handle)
{
    char value[512];
    if( false == GetProfileString(lpSecName, lpKeyName, value, sizeof(value), "0", handle))
        return false;
    char* p1 = strtok(value, ",");
    bool bResult = true;
    for(int i=0; i<nElements; i++) {
        if (p1 == NULL) {
            bResult = false;
            break;
        }
        pValue[i] = atof(p1);
        p1 = strtok(NULL, ",");
    }
    return bResult;
}
bool WriteProfileArrayInt(const char * lpSecName, const char * lpKeyName,  int* pValue, int nElements, void * handle)
{
    char szString[64];
    char szArray[512]={0};
    for (int i=0; i< nElements; i++) {
        sprintf(szString, " %d,", pValue[i]);
        strcat(szArray, szString);
    }
    return WriteProfileString(lpSecName, lpKeyName, szArray, handle);
}

bool GetProfileArrayInt(const char * lpSecName, const char * lpKeyName,   int* pValue, int nElements,  void * handle)
{
    char value[512];
    if( false == GetProfileString(lpSecName, lpKeyName, value, sizeof(value), "0", handle))
        return false;
    char* p1 = strtok(value, ",");
    bool bResult = true;
    for(int i=0; i<nElements; i++) {
        if (p1 == NULL) {
            bResult = false;
            break;
        }
        pValue[i] = atoi(p1);
        p1 = strtok(NULL, ",");
    }
    return bResult;
}


bool	WriteProfileRectFloat(const char * lpSecName, const char * lpKeyName,  nfRectF* pRect, void * handle)
{
    char szString[512];
    sprintf(szString, "%10.5f, %10.5f, %10.5f, %10.5f", pRect->l, pRect->t, pRect->r, pRect->b);
    return WriteProfileString(lpSecName, lpKeyName, szString, handle);

}
bool GetProfileRectFloat(const char * lpSecName, const char * lpKeyName,  nfRectF* pRect, void * handle)
{
    char value[256];
    if( false == GetProfileString(lpSecName, lpKeyName, value, sizeof(value), "0", handle))
        return false;

    char* p1 = strtok(value, ",");
    if(!p1) return false;
    pRect->l = atof(p1);
    p1 = strtok(NULL,",");
    if(!p1) return false;
    pRect->t = atof(p1);
    p1 = strtok(NULL,",");
    if(!p1) return false;
    pRect->r = atof(p1);
    p1 = strtok(NULL,",");
    if(!p1) return false;
    pRect->b = atof(p1);

    return true;
}
