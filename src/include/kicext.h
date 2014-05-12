/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************/

#ifdef __STDC__
extern void fatal_error(char*);
#else
extern void fatal_error();
#endif

/* 45s.c */
#ifdef __STDC__
extern void To45(int,int,int*,int*);
extern int  IsManhattan(int,int,int,int);
#else
extern void To45();
extern int  IsManhattan();
#endif

/* attri.c */
extern int NoMakeVisible;
#ifdef __STDC__
extern void Attri(int*);
extern void Updat(void);
extern void DisplayLabels(void);
extern void LabelInstances(void);
extern void Mark(void);
extern void Sides(void);
extern void AttribColor(int*);
extern void ShowRGB(void);
extern void SetColor(int,int);
extern void SetGrid(int*);
extern void Visib(int*);
extern void MakeLayerVisible(int);
extern void MakeLayerInvisible(int);
extern void Blink(int*);
extern void Dimen(int*);
extern void RemoveLayer(int*);
extern void AddLayer(void);
extern void Fill(int*);
extern int  RepaintFILL(void);
#else
extern void Attri();
extern void Updat();
extern void DisplayLabels();
extern void LabelInstances();
extern void Mark();
extern void Sides();
extern void AttribColor();
extern void ShowRGB();
extern void SetColor();
extern void SetGrid();
extern void Visib();
extern void MakeLayerVisible();
extern void MakeLayerInvisible();
extern void Blink();
extern void Dimen();
extern void RemoveLayer();
extern void AddLayer();
extern void Fill();
extern int  RepaintFILL();
#endif

/* basic.c */
#ifdef __STDC__
extern void Basic(int*);
extern void Rdraw(void);
extern void Undo(void);
extern void DoSet45(void);
extern void AbortKIC(void);
extern void ShowFull(void);
extern void Snap(void);
extern void Edit(int,int,int);
extern void TitleWindow(void);
extern void Save(void);
extern void WriteCell(void);
extern void Peek(void);
extern void Expand(void);
extern void CenterFullView(void);
extern int  FixCellName(char*);
#else
extern void Basic();
extern void Rdraw();
extern void Undo();
extern void DoSet45();
extern void AbortKIC();
extern void ShowFull();
extern void Snap();
extern void Edit();
extern void TitleWindow();
extern void Save();
extern void WriteCell();
extern void Peek();
extern void Expand();
extern void CenterFullView();
extern int  FixCellName();
#endif

/* boxes.c */
extern int MakingBoxes;
#ifdef __STDC__
extern int  InBox(int,int,struct ka*);
extern void ShowBox(int,struct ka*);
extern void ShowEmptyBox(int,struct ka*);
extern void EraseBox(struct ka*);
extern void Boxes(int*);
extern void OversizeBox(struct ka*,int);
extern void OutlineBox(struct ka*);
#else
extern int  InBox();
extern void ShowBox();
extern void ShowEmptyBox();
extern void EraseBox();
extern void Boxes();
extern void OversizeBox();
extern void OutlineBox();
#endif

/* break.c */
#ifdef __STDC__
extern void Break(int*);
extern struct p *CopyPath(struct p*);
#else
extern void Break();
extern struct p *CopyPath();
#endif

/* change.c */
#ifdef __STDC__
extern void ChangeLayer(int*);
#else
extern void ChangeLayer();
#endif

/* contexts.c */
#ifdef __STDC__
extern void Push(int*);
extern void ShowContext(void);
extern void UpdateParent(char*);
extern void Pop(void);
extern int  CheckModified(void);
extern void ClearContext(void);
#else
extern void Push();
extern void ShowContext();
extern void UpdateParent();
extern void Pop();
extern int  CheckModified();
extern void ClearContext();
#endif

/* copy.c */
#ifdef __STDC__
extern void Copy(int*);
extern void Move(int*);
extern void CopyPathWithXForm(struct p**);
extern void ShowMove(int,int,int,int);
extern void SetNewTransform(int,int,int,int);
#else
extern void Copy();
extern void Move();
extern void CopyPathWithXForm();
extern void ShowGhost();
extern void SetNewTransform();
#endif

/* debug.c */
#ifdef __STDC__
extern void Debug(int*);
extern void DoBW(void);
extern void DoAlloc(void);
#else
extern void Debug();
extern void DoBW();
extern void DoAlloc();
#endif

/* delete.c */
#ifdef __STDC__
extern void Del(int*);
#else
extern void Del();
#endif

/* dir.c */
#ifdef __STDC__
extern void Dir(void);
#else
extern void Dir();
#endif

/* erase.c */
#ifdef __STDC__
extern void Erase(int*);
extern void NewBox(int,int,int,int,int);
extern void NewPoly(Poly*,int);
#else
extern void Erase();
extern void NewBox();
extern void NewPoly();
#endif

/* flatten.c */
#ifdef __STDC__
extern void Flatten(int*);
#else
extern void Flatten();
#endif

/* grid.c */
#ifdef __STDC__
extern void ShowGrid(void);
extern void ShowAxes(void);
#else
extern void ShowGrid();
extern void ShowAxes();
#endif

/* hcopy.c */
#ifdef __STDC__
extern void Hcopy(void);
#else
extern void Hcopy();
#endif

/* init.c */
extern struct kv *View;
extern struct ka MenuViewport;
extern struct ka ParameterViewport;
extern struct ka LayerTableViewport;
extern struct kp Parameters;
extern struct kl LayerTable[CDNUMLAYERS+1];
extern struct kc KicCursor;
extern struct a  CurrentAOI;
extern int NumLayerTable;
extern char TypeOut[200];
#ifdef __STDC__
extern void Init(void);
extern void InitColorTable(void);
extern void InitVLT(void);
extern void InitParameters(void);
extern void DefaultWindows(void);
extern void InitCoarseWindow(int,int,int);
extern void InitFineWindow(int,int);
extern void SetPositioning(void);
extern void InitViewport(void);
extern void SetCurrentAOI(struct ka*);
#else
extern void Init();
extern void InitColorTable();
extern void InitVLT();
extern void InitParameters();
extern void DefaultWindows();
extern void InitCoarseWindow();
extern void InitFineWindow();
extern void SetPositioning();
extern void InitViewport();
extern void SetCurrentAOI();
#endif

/* instance.c */
#ifdef __STDC__
extern void Place(int*);
extern void Handle(void);
extern void ShowNewInstance(int,int,int,int);
extern void MakeInstance(int*,char*);
extern void GetArraySpec(void);
extern void NewSymbol(void);
extern int  OpenCell(char*,struct s**);
#else
extern void Place();
extern void Handle();
extern void ShowNewInstance();
extern void MakeInstance();
extern void GetArraySpec();
extern void NewSymbol();
extern int  OpenCell();
#endif

/* kicmain.c */
#ifdef __STDC__
extern void InitMenus(void);
extern void KICMain(void);
extern int  SafeCmds(int*);
extern char *NextCellName(void);
extern void SaveTechFile(void);
extern void InitSignals(void);
extern char *CopyString(char*);
extern char *tmalloc(unsigned);
extern void MallocFailed(void);
extern void UpdatePpoperties();
extern FILE *OpenDevice();
extern void Help(void);
extern void PutString(char*);
extern void PutErrorString(char*);
extern void PutBoldString(char*);
extern char *GetString(char*,int,FILE*,char*);
extern void RepaintWindow(int);
#else
extern void InitMenus();
extern void KICMain();
extern int  SafeCmds();
extern char *NextCellName();
extern void SaveTechFile();
extern void InitSignals();
extern char *CopyString();
extern char *tmalloc();
extern void MallocFailed();
extern void UpdatePpoperties();
extern FILE *OpenDevice();
extern void Help();
extern void PutString();
extern void PutErrorString();
extern void PutBoldString();
extern char *GetString();
extern void RepaintWindow();
#endif

/* labels.c */
#ifdef __STDC__
extern void Label(int*);
extern void BBLabel(struct ka*,struct o*,struct ka*);
extern void CDLabelBB(struct o*,int*,int*,int*,int*);
extern void ShowLabel(int,char*,int,int,int,int);
extern char SetXform(int*);
#else
extern void Label();
extern void BBLabel();
extern void CDLabelBB();
extern void ShowLabel();
extern char SetXform();
#endif

/* lineclip.c */
#ifdef __STDC__
extern void Y_Intercept(int,int,int,int,int,int*);
extern void X_Intercept(int,int,int,int,int,int*);
extern int  LineClip(int*,int*,int*,int*,int,int,int,int);
#else
extern void Y_Intercept();
extern void X_Intercept();
extern int  LineClip();
#endif

/* lines.c */
#ifdef __STDC__
extern void ShowLine(int,int,int,int,int);
extern void ShowManhattanLine(int,int,int,int,int);
#else
extern void ShowLine();
extern void ShowManhattanLine();
#endif

/* logo.c */
#ifdef __STDC__
extern void Logo(int*);
#else
extern void Logo();
#endif

/* measure.c */
#ifdef __STDC__
extern void StartTiming(void);
extern void StopTiming(void);
extern int  ElapsedRealTime(void);
extern int  ElapsedUserTime(void);
extern int  ElapsedSystemTime(void);
extern void ShowRatio(char*,int,char*,int);
#else
extern void StartTiming();
extern void StopTiming();
extern int  ElapsedRealTime();
extern int  ElapsedUserTime();
extern int  ElapsedSystemTime();
extern void ShowRatio();
#endif

/* modify.c */
#ifdef __STDC__
extern void Stretch(int*);
extern void SetStretchMode(void);
extern void ShowStretch(int,int,int,int);
#else
extern void Stretch();
extern void SetStretchMode();
extern void ShowStretch();
#endif

/* more.c */
#ifdef __STDC__
extern int  MoreLine(char*);
extern void EnableMore(int);
extern int  RepaintMore(void);
extern int  MorePageDisplay(void);
#else
extern int  MoreLine();
extern void EnableMore();
extern int  RepaintMore();
extern int  MorePageDisplay();
#endif

/* point.c */
extern int LockOut;
#ifdef __STDC__
extern void Point(void);
extern int  PointLoop(int*);
extern int  PointLoopCreate(int*);
extern int  PointLoopSafe(int*);
extern int  PointLoopLayer(int*);
extern void NotPointingAtLayout(void);
extern void RedisplayKIC(void);
extern void FullRedisplay(void);
extern void FinePosition(int,int,int);
#else
extern void Point();
extern int  PointLoop();
extern int  PointLoopCreate();
extern int  PointLoopSafe();
extern int  PointLoopLayer();
extern void NotPointingAtLayout();
extern void RedisplayKIC();
extern void FullRedisplay();
extern void FinePosition();
#endif

/* polyclip.c */
#ifdef __STDC__
extern void PolygonClip(Poly*,int,int,int,int);
extern int  NewPolygon(Poly*);
#else
extern void PolygonClip();
extern int  NewPolygon();
#endif

/* polygns.c */
#ifdef __STDC__
extern void Polygons(int*);
extern void Flash(int*);
extern void Doughnut(int*);
extern void Arcs(int*);
extern void ShowPath(int,struct p*,int);
extern void ShowPolygon(int,struct p*);
extern void LastPointInPath(int*,int*,struct p*);
extern struct p *AllocatePath(int,int);
#else
extern void Polygons();
extern void Flash();
extern void Doughnut();
extern void Arcs();
extern void ShowPath();
extern void ShowPolygon();
extern void LastPointInPath();
extern struct p *AllocatePath();
#endif

/* prpty.c */
#ifdef __STDC__
extern void Properties(int*);
extern void DoShowProperties(void);
extern void AddProperty(void);
extern void RemoveProperty(void);
extern void RemovePropertyList(struct o*,struct prpty**);
extern void RestorePropertyList(struct o*,struct prpty*);
#else
extern void Properties();
extern void DoShowProperties();
extern void AddProperty();
extern void RemoveProperty();
extern void RemovePropertyList();
extern void RestorePropertyList();
#endif

/* redispla.c */
#ifdef __STDC__
extern void Redisplay(struct ka*);
extern void RedisplayAfterInterrupt(void);
extern int  TCheck(void);
extern void SetTransform(struct o*);
#else
extern void Redisplay();
extern void RedisplayAfterInterrupt();
extern int  TCheck();
extern void SetTransform();
#endif

/* select.c */
extern struct ka SelectQBB;
extern struct ks *SelectQHead;
#ifdef __STDC__
extern void MX(void);
extern void MY(void);
extern void Rotat0(void);
extern void Rotat90(void);
extern void Rotat180(void);
extern void Rotat270(void);
extern void Sel(int*);
extern void Area(int*);
extern void Desel(void);
extern int  Layer(void);
extern void Selection(struct ka*);
extern struct ks *SelectItems(struct ka*,int);
extern void SLFree(struct ks*);
extern void SLBB(struct ks*,struct ka*);
extern void GetBB(struct o*,struct ka*);
extern int  BBVisible(struct o*);
extern int  AreTypesInQ(char*);
extern void SelectTypes(char*);
extern void SQInit(void);
extern void SQClear(void);
extern void SQInsert(struct o*);
extern void SQDelete(struct o*);
extern void SQComputeBB(void);
extern void SQRestore(int);
extern void SQDesel(char*);
extern void SQShow(void);
extern int *InPath(int,struct p*,int,int);
#else
extern void MX();
extern void MY();
extern void Rotat0();
extern void Rotat90();
extern void Rotat180();
extern void Rotat270();
extern void Sel();
extern void Area();
extern void Desel();
extern int  Layer();
extern void Selection();
extern struct ks *SelectItems();
extern void SLFree();
extern void SLBB();
extern void GetBB();
extern int  BBVisible();
extern int  AreTypesInQ();
extern void SelectTypes();
extern void SQInit();
extern void SQClear();
extern void SQInsert();
extern void SQDelete();
extern void SQComputeBB();
extern void SQRestore();
extern void SQDesel();
extern void SQShow();
extern int *InPath();
#endif

/* sline.c */
#ifdef __STDC__
extern void sline();
#else
extern void sline();
#endif

/* techfile.c */
extern int FineVPonBottom;
extern char InitScreenMode;
#ifdef __STDC__
extern void ReadTechFile(void);
#else
extern void ReadTechFile();
#endif

/* viewport.c */
#ifdef __STDC__
extern void PToL(struct ka*,int*,int*);
extern void ClipToGridPoint(int*,int*);
extern void ShowParameters(void);
extern void SetRelative(int,int,int);
extern void ShowElectrical(void);
extern void ShowLayerTable(void);
extern void LtMore(void);
extern void LtBox(int,int,int,int);
extern int  PointLayerTable(int,int);
extern void ShowCommandMenu(void);
extern void ShowMenu(MENU*);
extern int  GetMenuIndex(MENU*,char*); 
extern MENU *GetCurrentMenu(void);
extern void AlterMenuEntries(char*,char*);
extern void MenuSelect(char*);
extern void MenuDeselect(char*);
extern void FixMenuPrefix(MENU*);
extern void FixMenuEntryPrefix(MENU*,int);
extern void ShowPrompt(char*);
extern void ShowPromptAndWait(char*);
extern void ShowPromptWithColor(char*,int);
extern void RedrawPrompt(void);
extern void AppendToOldPrompt(int);
extern void ErasePrompt(void);
extern void OutlineText(int,int,int,int,int,int,int);
extern void EraseLargeCoarseViewport(void);
extern void RedisplayViewports(void);
extern void ShowFineViewport(void);
extern void ShowCurrentObject(struct o*,int);
extern void ShowInstanceMarker(int,int,struct o*);
extern void ShowMarker(int,int,int,int,int);
extern void ShowProcess(char*);
#else
extern void PToL();
extern void ClipToGridPoint();
extern void ShowParameters();
extern void SetRelative();
extern void ShowElectrical();
extern void ShowLayerTable();
extern void LtMore();
extern void LtBox();
extern int  PointLayerTable();
extern void ShowCommandMenu();
extern void ShowMenu();
extern int  GetMenuIndex(); 
extern MENU *GetCurrentMenu();
extern void AlterMenuEntries();
extern void MenuSelect();
extern void MenuDeselect();
extern void FixMenuPrefix();
extern void FixMenuEntryPrefix();
extern void ShowPrompt();
extern void ShowPromptAndWait();
extern void ShowPromptWithColor();
extern void RedrawPrompt();
extern void AppendToOldPrompt();
extern void ErasePrompt();
extern void OutlineText();
extern void EraseLargeCoarseViewport();
extern void RedisplayViewports();
extern void ShowFineViewport();
extern void ShowCurrentObject();
extern void ShowInstanceMarker();
extern void ShowMarker();
extern void ShowProcess();
#endif

/* wires.c */
#ifdef __STDC__
extern void Wires(int*);
extern void ShowWire(int,int,struct p*);
extern void Width(int*);
extern void RemoveLastPointInPath(struct p**);
extern void AppendPointToPath(int*,int*,struct p**);
#else
extern void Wires();
extern void ShowWire();
extern void Width();
extern void RemoveLastPointInPath();
extern void AppendPointToPath();
#endif

/* xorbox.c */
#ifdef __STDC__
extern void XORbox(int*);
#else
extern void XORbox();
#endif

/* zoom.c */
#ifdef __STDC__
extern void Pan(int*);
extern void Zoom(int*);
extern void Windo(int*);
extern void LastView(void);
extern void RestoreLastView(void);
extern void SaveLastView(void);
extern void SaveViewOnStack(void);
#else
extern void Pan();
extern void Zoom();
extern void Windo();
extern void LastView();
extern void RestoreLastView();
extern void SaveLastView();
extern void SaveViewOnStack();
#endif

/* database conversion */

/* convert.c */
#ifdef __STDC__
extern void Convert(void);
extern void OutPrompt(char*);
extern void ToGDSII(void);
extern void ConvertPathtype(int*,int*,int,int,int,int);
extern void ToCIF(void);
extern void FromCIF(void);
#else
extern void Convert();
extern void OutPrompt();
extern void ToGDSII();
extern void ConvertPathtype();
extern void ToCIF();
extern void FromCIF();
#endif

/* convert1.c */
#ifdef __STDC__
extern void FromGDSII(void);
#else
extern void FromGDSII();
#endif

