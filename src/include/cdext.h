/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************/

/* actions.c */
#if __STDC__
extern void AEnd(void);
extern int  ABeginSymbol(int,int,int);
extern void AEndSymbol(void);
extern void ADeleteSymbol(int);
extern int  AEndCall(void);
extern int  AT(int,int,int);
extern int  ABeginCall(int);
extern int  APolygon(struct p*);
extern int  AWire(int,struct p*);
extern int  ABox(int,int,int,int,int,int);
extern int  ARoundFlash(int,int,int);
extern int  ALayer(int,char*);
extern int  AUserExtension(int,char*);
extern void AComment(char*);
extern int  AMallocFailed(void);
#else
extern void AEnd();
extern int  ABeginSymbol();
extern void AEndSymbol();
extern void ADeleteSymbol();
extern int  AEndCall();
extern int  AT();
extern int  ABeginCall();
extern int  APolygon();
extern int  AWire();
extern int  ABox();
extern int  ARoundFlash();
extern int  ALayer();
extern int  AUserExtension();
extern void AComment();
extern int  AMallocFailed();
#endif

/* cd.c */
extern struct bu *CDSymbolTable[CDNUMLAYERS+1];
extern struct d  CDDesc;
extern struct l  CDLayer[CDNUMLAYERS+1];
extern char *CDStatusString;
extern int  CDStatusInt;
#if __STDC__
extern int  CDInit(void);
extern int  CDPath(char*);
extern void CDSetLayer(int,int,char*);
extern void CDDebug(int);
extern int  CDOpen(char*,struct s**,int);
extern void CDSymbol(char*,struct s**);
extern int  CDClose(struct s*);
extern int  CDReflect(struct s*);
extern int  CDPatchInstances(struct s*,char*);
extern int  CDMakeBox(struct s*,int,int,int,int,int,struct o**);
extern int  CDMakeLabel(struct s*,int,char*,int,int,int,struct o**);
extern int  CDMakePolygon(struct s*,int,struct p*,struct o**);
extern int  CDMakeWire(struct s*,int,int,struct p*,struct o**);
extern int  CDMakeRoundFlash(struct s*,int,int,int,int,struct o**);
extern int  CDBeginMakeCall(struct s*,char*,int,int,int,int,struct o**);
extern int  CDT(struct o*,int,int,int);
extern int  CDEndMakeCall(struct s*,struct o*);
extern void CDCheckPath(struct p*);
extern int  CDInsertObjectDesc(struct s*,struct o*);
extern void CDDeleteObjectDesc(struct s*,struct o*);
extern void CDCall(struct o*,char**,int*,int*,int*,int*);
extern void CDBox(struct o*,int*,int*,int*,int*,int*);
extern void CDLabel(struct o*,int*,char**,int*,int*,char*);
extern void CDPolygon(struct o*,int*,struct p**);
extern void CDWire(struct o*,int*,int*,struct p**);
extern void CDRoundFlash(struct o*,int*,int*,int*,int*);
extern void CDInfo(struct s*,struct o*,int*);
extern void CDSetInfo(struct s*,struct o*,int);
extern void CDProperty(struct s*,struct o*,struct prpty**);
extern int  CDAddProperty(struct s*,struct o*,int,char*);
extern int  CDRemoveProperty(struct s*,struct o*,int);
extern void CDType(struct o*,char*);
extern int  CDBB(struct s*,struct o*,int*,int*,int*,int*);
extern void CDIntersect(int,int,int,int,int*,int*,int*,int*);
extern int  CDInitGen(struct s*,int,int,int,int,int,struct g**);
extern void CDGen(struct s*,struct g*,struct o**);
extern void CDInitTGen(struct o*,struct t**);
extern void CDTGen(struct t**,char*,int*,int*);
extern int  CDUpdate(struct s*,char*);
extern int  CDGenCIF(FILE*,struct s*,int*,int,int,int);
extern int  CDTo(char*,char*,int,int,int);
extern int  CDFrom(char*,char*,int,int,int*,int,int);
extern int  CDParseCIF(char*,char*,int);
extern int  CDUnmark(struct s*);
extern int  CDError(int);
#else
extern int  CDInit();
extern int  CDPath();
extern void CDSetLayer();
extern void CDDebug();
extern int  CDOpen();
extern void CDSymbol();
extern int  CDClose();
extern int  CDReflect();
extern int  CDPatchInstances();
extern int  CDMakeBox();
extern int  CDMakeLabel();
extern int  CDMakePolygon();
extern int  CDMakeWire();
extern int  CDMakeRoundFlash();
extern int  CDBeginMakeCall();
extern int  CDT();
extern int  CDEndMakeCall();
extern void CDCheckPath();
extern int  CDInsertObjectDesc();
extern void CDDeleteObjectDesc();
extern void CDCall();
extern void CDBox();
extern void CDLabel();
extern void CDPolygon();
extern void CDWire();
extern void CDRoundFlash();
extern void CDInfo();
extern void CDSetInfo();
extern void CDProperty();
extern int  CDAddProperty();
extern int  CDRemoveProperty();
extern void CDType();
extern int  CDBB();
extern void CDIntersect();
extern int  CDInitGen();
extern void CDGen();
extern void CDInitTGen();
extern void CDTGen();
extern int  CDUpdate();
extern int  CDGenCIF();
extern int  CDTo();
extern int  CDFrom();
extern int  CDParseCIF();
extern int  CDUnmark();
extern int  CDError();
#endif

/* gencif.c */
#if __STDC__
extern void GenEnd(FILE*);
extern void GenBeginSymbol(FILE*,int,int,int);
extern void GenEndSymbol(FILE*);
extern void GenBeginCall(FILE*,int);
extern void GenEndCall(FILE*);
extern void GenTranslation(FILE*,int,int);
extern void GenRotation(FILE*,int,int);
extern void GenMirrorX(FILE*);
extern void GenMirrorY(FILE*);
extern void GenPolygon(FILE*,struct p*);
extern void GenPolygonOffset(FILE*,struct p*,int,int);
extern void GenWire(FILE*,int,struct p*);
extern void GenWireOffset(FILE*,int,struct p*,int,int);
extern void GenBox(FILE*,int,int,int,int,int,int);
extern void GenLayer(FILE*,int,char*);
extern void GenUserExtension(FILE*,int,char*);
extern void GenComment(FILE*,char*);
#else
extern void GenEnd();
extern void GenBeginSymbol();
extern void GenEndSymbol();
extern void GenBeginCall();
extern void GenEndCall();
extern void GenTranslation();
extern void GenRotation();
extern void GenMirrorX();
extern void GenMirrorY();
extern void GenPolygon();
extern void GenPolygonOffset();
extern void GenWire();
extern void GenWireOffset();
extern void GenBox();
extern void GenLayer();
extern void GenUserExtension();
extern void GenComment();
#endif

/* paths.c */
#if __STDC__
extern int PConvertTilde(char**,char**,int*);
extern int PSetPath(char*);
extern char *PGetPath(void);
extern FILE *POpen(char*,char*,char*,char**);
#else
extern int PConvertTilde();
extern int PSetPath();
extern char *PGetPath();
extern FILE *POpen();
#endif

/* xforms.c */
#if __STDC__
extern void TInit(void);
extern int  TEmpty(void);
extern int  TFull(void);
extern void TPush(void);
extern void TPop(void);
extern void TCurrent(int*);
extern void TLoadCurrent(int*);
extern void TTranslate(int,int);
extern void TMY(void);
extern void TMX(void);
extern void TRotate(int,int);
extern void TIdentity(void);
extern void TPoint(int*,int*);
extern void TPremultiply(void);
extern void TInverse(void);
extern void TInversePoint(int*,int*);
extern void TStore(void);
extern void TLoad(void);
extern void TLoadInverse(void);
#else
extern void TInit();
extern int  TEmpty();
extern int  TFull();
extern void TPush();
extern void TPop();
extern void TCurrent();
extern void TLoadCurrent();
extern void TTranslate();
extern void TMY();
extern void TMX();
extern void TRotate();
extern void TIdentity();
extern void TPoint();
extern void TPremultiply();
extern void TInverse();
extern void TInversePoint();
extern void TStore();
extern void TLoad();
extern void TLoadInverse();
#endif
