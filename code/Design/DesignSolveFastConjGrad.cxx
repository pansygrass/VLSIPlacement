# include <Design.h>
# include <ConjGradSolver.h>

void
getObjFuncInConjGradFormat(Design &myDesign, HyperGraph &myGraph,
			   vector<Cell *> &inputCells, 
			   SpMat **matX, SpMat **matY, 
			   double **bx, double **by,
			   double &constantx, double &constanty,
			   map<Cell *, uint>& quadMap,
			   map<Cell *, uint>& linMap,
			   Env &DesignEnv)
{
  vector<uint> subi_vecx, subj_vecx, sub_vecx;
  vector<uint> subi_vecy, subj_vecy, sub_vecy;
  vector<double> valij_vecx, val_vecx;
  vector<double> valij_vecy, val_vecy;
  uint numVars, i;
  ofstream hessDumpFile;
  ofstream bMatDumpFile;
  EnvNetModel netModel = DesignEnv.EnvGetNetModel();

  if (netModel == ENV_CLIQUE_MODEL) {
    getObjectiveCliqueModelXY(myDesign, myGraph, inputCells, 
			      subi_vecx, subj_vecx, valij_vecx, 
			      sub_vecx, val_vecx, 
			      subi_vecy, subj_vecy, valij_vecy, 
			      sub_vecy, val_vecy, 
			      constantx, constanty, quadMap, linMap);
  } else if (netModel == ENV_STAR_MODEL) {
    getObjectiveStarModelXY(myDesign, myGraph, inputCells, 
			      subi_vecx, subj_vecx, valij_vecx, 
			      sub_vecx, val_vecx, 
			      subi_vecy, subj_vecy, valij_vecy, 
			      sub_vecy, val_vecy, 
			      constantx, constanty, quadMap, linMap);
  }    
  numVars = inputCells.size();
  *matX = new SpMat(subi_vecx, subj_vecx, valij_vecx);
  *matY = new SpMat(subi_vecy, subj_vecy, valij_vecy);
  *bx = (double *) malloc(sizeof(double) * numVars);
  *by = (double *) malloc(sizeof(double) * numVars);
  
  for (i = 0; i < numVars; i++) {
    (*bx)[i] = 0.0;
    (*by)[i] = 0.0;
  }
  for (i = 0; i < sub_vecx.size(); i++) {
    (*bx)[sub_vecx[i]] = -val_vecx[i];
  }
  for (i = 0; i < sub_vecy.size(); i++) {
    (*by)[sub_vecy[i]] = -val_vecy[i];
  }

  /* Dump the hessian and the constant matrices into a file */
  if (DesignEnv.EnvGetDumpHessian()) {
    string hessDir, hessFile, bMatFile;
    hessDir = "/home/nakul/thesis/VLSIPlacement/code/SolverTest/" + DesignEnv.EnvGetDesignName();
    if (!dirExists(hessDir)) {
      if (!(0 == mkdir(hessDir.data(), S_IRWXU | S_IRWXG | S_IRWXO))) {
	cout << "Error: Directory does not exist. Cannot create directory!!" << endl;
	exit(0);
      }
    }
    hessFile = hessDir + "/hessianx";
    if (DesignEnv.EnvGetNetModel() == ENV_STAR_MODEL) {
      hessFile = hessFile + "star";
    } else if (DesignEnv.EnvGetNetModel() == ENV_CLIQUE_MODEL) {
      hessFile = hessFile + "clique";
    }
    hessFile += ".txt";
    hessDumpFile.open(hessFile.data());
    uint numNonZero;
    numNonZero = valij_vecx.size();
    for (uint i = 0; i < numNonZero; i++) {
      hessDumpFile << subi_vecx[i] << " " << subj_vecx[i] << " " << valij_vecx[i] << endl;
    }
    hessDumpFile.close();

    hessFile = hessDir + "/hessiany";
    if (DesignEnv.EnvGetNetModel() == ENV_STAR_MODEL) {
      hessFile = hessFile + "star";
    } else if (DesignEnv.EnvGetNetModel() == ENV_CLIQUE_MODEL) {
      hessFile = hessFile + "clique";
    }
    hessFile += ".txt";
    hessDumpFile.open(hessFile.data());
    numNonZero = valij_vecx.size();
    for (uint i = 0; i < numNonZero; i++) {
      hessDumpFile << subi_vecy[i] << " " << subj_vecy[i] << " " << valij_vecy[i] << endl;
    }
    hessDumpFile.close();

    bMatFile = hessDir + "/bmatx";
    if (DesignEnv.EnvGetNetModel() == ENV_STAR_MODEL) {
      bMatFile = bMatFile + "star";
    } else if (DesignEnv.EnvGetNetModel() == ENV_CLIQUE_MODEL) {
      bMatFile = bMatFile + "clique";
    }
    bMatFile += ".txt";
    bMatDumpFile.open(bMatFile.data());
    numNonZero = val_vecx.size();
    for (uint i = 0; i < numNonZero; i++) {
      bMatDumpFile << sub_vecx[i] << " " << val_vecx[i] << endl;
    }
    bMatDumpFile.close();

    bMatFile = hessDir + "/bmaty";
    if (DesignEnv.EnvGetNetModel() == ENV_STAR_MODEL) {
      bMatFile = bMatFile + "star";
    } else if (DesignEnv.EnvGetNetModel() == ENV_CLIQUE_MODEL) {
      bMatFile = bMatFile + "clique";
    }
    bMatFile += ".txt";
    bMatDumpFile.open(bMatFile.data());
    numNonZero = val_vecy.size();
    for (uint i = 0; i < numNonZero; i++) {
      bMatDumpFile << sub_vecy[i] << " " << val_vecy[i] << endl;
    }
    bMatDumpFile.close();
  }
}

void
addCellToBin(Cell *cellPtr)
{
  
}

void
Design::DesignSolveForAllCellsConjGradIter(void)
{
  void *cellObj;
  ofstream logFile;
  vector<Cell *> inputCells;
  map<Cell *, uint> quadMap;
  map<Cell *, uint>::iterator quadMapItr;
  map<uint, double> quadMapX, quadMapY;
  map<Cell *, uint> linMap;
  map<Cell *, uint>::iterator linMapItr;
  map<uint, double> linMapX, linMapY;
  string DesignPath, DesignName, logFileName;
  string DirName;
  string plotFileName;
  double prevPeakUtil, itrCount, stopThreshold;
  uint cellLeft, cellRight, cellTop, cellBot;
  uint cellHeight, cellWidth;
  uint peakUtilBinIdx;
  uint numVars, nodeIdx;
  uint qvalIdx, i;
  SpMat *matX, *matY;
  double *bx, *by;
  double constantx, constanty;
  
  /* All the initilizations */
  Env &DesignEnv = (*this).DesignEnv;
  DesignPath = DesignEnv.EnvGetDesignPath();
  DirName = DesignPath + "/.solverData";
  DesignName = DesignEnv.EnvGetDesignName();

  HyperGraph &myGraph = (*this).DesignGetGraph();

  /* Insert cells into a vector in the order of their indices in the 
     hypergraph */ 
  HYPERGRAPH_FOR_ALL_NODES(myGraph, nodeIdx, cellObj) {
    if ((*(Cell*)cellObj).CellIsTerminal()) continue;
    inputCells.push_back((Cell *)cellObj);
    cellsToSolve.push_back((Cell *)cellObj);
  } HYPERGRAPH_END_FOR;

  DesignSetCellsToSolve(inputCells);

  getObjFuncInConjGradFormat((*this), myGraph, inputCells, &matX, &matY, 
			     &bx, &by, constantx, constanty, quadMap,
			     linMap, DesignEnv);

  /* STORE THE ORIGINAL VALUES OF THE QUADRATIC TERMS IN A MAP */
  /* THIS WOULD BE A KEY-VALUE: INDEX-VALUE PAIR WHICH WOULD   */
  /* THEN BE USED TO REPLACE THE VALUES IN THE QVALIJ MATRIX   */
  Cell *cellPtr;
  MAP_FOR_ALL_ELEMS(quadMap, Cell*, uint, cellPtr, qvalIdx) {
    quadMapX[qvalIdx] = (*matX).GetValue(qvalIdx);
    quadMapY[qvalIdx] = (*matY).GetValue(qvalIdx);
  } END_FOR;
  MAP_FOR_ALL_ELEMS(linMap, Cell*, uint, cellPtr, qvalIdx) {
    linMapX[qvalIdx] = bx[qvalIdx];
    linMapY[qvalIdx] = by[qvalIdx];
  } END_FOR;
  
  numVars = inputCells.size();
  double x[numVars];
  double y[numVars];
  double eps=1e-13;
  uint max_iterations = 2000;
  for (i = 0; i < numVars; i++) {
    x[i] = 0.0;
    y[i] = 0.0;
  }

  /**************************************************************/
  /* LOOP OF QUADRATIC SOLVING AND SPREADING BEINGS HERE        */
  /**************************************************************/
  prevPeakUtil = 0.0;
  itrCount = 0;
  stopThreshold = 0.1;
  _STEP_BEGIN("Analytical solve and spread iterations");
  plotFileName = DesignName + ".bins.plt";
  DesignPlotData("Title", plotFileName);
  DesignComputeBinSize(false);
  //  DesignCreateEmptyBins();
  while (1) {
    /**************************************************************/
    /* SOLVER PART BEGINS HERE  :  SOLVE FOR X FIRST AND THEN Y   */
    /**************************************************************/
    _STEP_BEGIN("Solve for X using Conjugate Gradient minimization");
    cghs(numVars, (*matX), bx, x, eps, false, max_iterations);
    _STEP_END("Solve for X using Conjugate Gradient minimization");
    /**************************************************************/
    /* SOLVE FOR Y NEXT                                           */
    /**************************************************************/
    _STEP_BEGIN("Solve for Y using Conjugate Gradient minimization");
    cghs(numVars, (*matY), by, y, eps, false, max_iterations);
    _STEP_END("Solve for Y using Conjugate Gradient minimization");
    /**************************************************************/
    /* WRITE CELL LOCATIONS TO LOG FILE IN EACH ITERATION         */
    /* IN THE TEST MODE ONLY                                      */
    /**************************************************************/
    if (DesignEnv.EnvGetToolMode() == ENV_MODE_TEST) {
      if (!dirExists(DirName)) {
	if (!(0 == mkdir(DirName.data(), S_IRWXU | S_IRWXG | S_IRWXO))) {
	  cout << "Error: Directory does not exist. Cannot create directory!!" << endl;
	  exit(0);
	}
      }
      logFileName = DirName + "/ConjGradX_Itr" + getStrFromInt(itrCount) + ".txt";
      logFile.open(logFileName.data());
      for (i = 0; i < numVars; i++) {
	logFile << "X" << i << ": " << x[i] << endl;
      }
      logFile.close();
      logFileName = DirName + "/ConjGradY_Itr" + getStrFromInt(itrCount) + ".txt";
      logFile.open(logFileName.data());
      for (i = 0; i < numVars; i++) {
	logFile << "Y" << i << ": " << y[i] << endl;
      }
      logFile.close();
    }
    /**************************************************************/
    /* ASSIGN LOCATIONS TO CELLS                                  */
    /**************************************************************/
    _STEP_BEGIN("Assigning locations to cells");
    Cell *cellPtr;
    double xpos, ypos;
    for (i = 0; i < numVars; i++) {
      cellPtr = inputCells[i];
      xpos = x[i] * GRID_COMPACTION_RATIO; 
      ypos = y[i] * GRID_COMPACTION_RATIO;
      cellWidth = (*cellPtr).CellGetWidth();
      cellHeight = (*cellPtr).CellGetHeight();
      cellRight = xpos + cellWidth;
      cellTop = xpos + cellHeight;
      cellBot = ypos; cellLeft = xpos;

      if (cellRight > maxx) { xpos -= (cellRight - maxx); }
      if (cellTop > maxy) { ypos -= (cellTop - maxy); }
      if (xpos < 0) { xpos = 0; }
      if (ypos < 0) { ypos = 0; }
      if (xpos > maxx) { xpos = maxx - cellWidth - 1; }
      if (ypos > maxy) { ypos = maxy - cellHeight - 1; }
      (*cellPtr).CellSetXposDbl(xpos);
      (*cellPtr).CellSetYposDbl(ypos);
      CellSetDblX(cellPtr, xpos);
      CellSetDblY(cellPtr, ypos);
    }
    _STEP_END("Assigning locations to cells");

    /**************************************************************/
    /* BIN CREATION AND STRETCHING                                */
    /**************************************************************/
    _STEP_BEGIN("Creating Bins");
    DesignCreateBins();
    _STEP_END("Creating Bins");

    _STEP_BEGIN("Stretching Bins");
    DesignStretchBins();
    _STEP_END("Stretching Bins");
    
    /**************************************************************/
    /* PRINT THE PEAK UTILIZATION VALUE                           */
    /**************************************************************/
    peakUtilization = DesignGetPeakUtil();
    peakUtilBinIdx = DesignGetPeakUtilBinIdx();
    if (prevPeakUtil == 0.0) {
       prevPeakUtil = peakUtilization;
    }
    cout << "Iteration: " << itrCount++
	 << " Peak Utilization: " << peakUtilization
	 << " Bin index: " << peakUtilBinIdx 
	 << " Mem usage: " << getMemUsage() << MEM_USAGE_UNIT
	 << " CPU TIME:" << getCPUTime() << CPU_TIME_UNIT << endl;

    /**************************************************************/
    /* STOPPING CONDITION                                         */
    /**************************************************************/
    if ((itrCount < 0) || 
	((prevPeakUtil > peakUtilization) && 
	 ((prevPeakUtil - peakUtilization) < stopThreshold))) {
      cout << "Global placement complete" << endl;
      plotFileName = DesignName + ".gp.plt";
      DesignPlotData("Title", plotFileName);
      break;
    } else {
      prevPeakUtil = peakUtilization;
    }

    /**************************************************************/
    /* RESET LINEAR AND QUADRATIC VALUES                          */
    /**************************************************************/
    _STEP_BEGIN("Reset quadratic and linear term values");
    MAP_FOR_ALL_ELEMS(quadMap, Cell*, uint, cellPtr, qvalIdx) {
      (*matX).SetValue(qvalIdx, quadMapX[qvalIdx]);
      (*matY).SetValue(qvalIdx, quadMapY[qvalIdx]);
    } END_FOR;
    MAP_FOR_ALL_ELEMS(linMap, Cell*, uint, cellPtr, qvalIdx) {
      bx[qvalIdx] = linMapX[qvalIdx];
      by[qvalIdx] = linMapY[qvalIdx];
    } END_FOR;
    _STEP_END("Reset quadratic and linear term values");

    /**************************************************************/
    /* ADD SPREADING FORCES                                       */
    /**************************************************************/
    _STEP_BEGIN("Perform cell spreading");
    double cellXpos, cellYpos;
    double pseudoPinX, pseudoPinY;
    double springConstant;
    double coeffX, coeffY;
    uint quadCellIdx, linCellIdx;
    for (i = 0; i < numVars; i++) {
      Cell &thisCell = (*(Cell *)(inputCells[i]));
      if (CellIsStarNode(&thisCell)) break;
      Bin *binPtr = (Bin *)CellGetBin(&thisCell);
      cellXpos = thisCell.CellGetXposDbl();
      cellYpos = thisCell.CellGetYposDbl();
      //      cellXpos = CellGetDblX(&thisCell);
      //   cellYpos = CellGetDblY(&thisCell);
      DesignSpreadCreatePseudoPort(thisCell, *binPtr, cellXpos, cellYpos,
                                   pseudoPinX, pseudoPinY, springConstant);
      //      cout << "Created pseudo port at: (" << pseudoPinX << "," << pseudoPinY << ")   Spring constant:" << springConstant << endl;
      coeffX = springConstant; coeffY = coeffX;
      _KEY_EXISTS_WITH_VAL(quadMap, (&thisCell), quadMapItr) {
        quadCellIdx = quadMapItr->second;
      } else {
        cout << "SEVERE ERROR QUAD: ENTRY FOR cell: (PTR: " << &thisCell << ") "
             << thisCell.CellGetName() << " not found in quadMap" << endl;
        exit(0);
      }
      _KEY_EXISTS_WITH_VAL(linMap, (&thisCell), linMapItr) {
        linCellIdx = linMapItr->second;
      } else {
        cout << "SEVERE ERROR LINEAR: ENTRY FOR cell: (PTR: " << &thisCell << ") "
             << thisCell.CellGetName() << " not found in linMap" << endl;
        exit(0);
      }
      /* Update the diagonals of the quadratic matrix */
      //      cout << "Updated the quad value of cell index " << quadCellIdx << " from " << (*matX).GetValue(quadCellIdx);
      (*matX).AddValue(quadCellIdx, coeffX);
      //      cout << " to " << (*matX).GetValue(quadCellIdx) << endl;
      //      cout << "Updated the quad value of cell index " << quadCellIdx << " from " << (*matY).GetValue(quadCellIdx);
      (*matY).AddValue(quadCellIdx, coeffY);
      //cout << " to " << (*matY).GetValue(quadCellIdx) << endl;
      
      /* Update the linear matrix */
      coeffX = coeffX * (pseudoPinX / GRID_COMPACTION_RATIO);
      coeffY = coeffY * (pseudoPinY / GRID_COMPACTION_RATIO);
      //      cout << "Updated the lin value of cell index " << linCellIdx << " from " << bx[linCellIdx];
      bx[i] += (coeffX);
      //      cout << " to " << bx[linCellIdx] << endl;
      //      cout << "Updated the lin value of cell index " << linCellIdx << " from " << by[linCellIdx];
      by[i] += (coeffY);
      //      cout << " to " << by[linCellIdx] << endl;
    }
    _STEP_END("Perform cell spreading");

    /**************************************************************/
    /* REMOVE BINS                                                */
    /**************************************************************/
    _STEP_BEGIN("Perform bin removal");
    DesignClearBins();
    _STEP_END("Perform bin removal");

    //    for (i = 0; i < numVars; i++) {
    //      x[i] = 0.0;
    //      y[i] = 0.0;
    //    }
  }
  _STEP_END("Analytical solve and spread iterations");
}
