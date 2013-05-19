# include <Design.h>
# define FORCE_DIR_NO_FORCE 0
# define FORCE_DIR_FIRST_QUAD 1
# define FORCE_DIR_SECOND_QUAD 2
# define FORCE_DIR_THIRD_QUAD 3
# define FORCE_DIR_FOURTH_QUAD 4
# define FORCE_DIR_RIGHT 5
# define FORCE_DIR_LEFT 6
# define FORCE_DIR_TOP 7
# define FORCE_DIR_BOT 8
# define MAX_DBL 1000000000000000

void
Design::DesignGetForceOnCell(Cell &thisCell,
			     double newXPos, double newYPos,
			     double &magnitude,
			     double &totalXForce, double &totalYForce,
			     char &forceDir, double &chipBoundLeft,
			     double &chipBoundRight, double &chipBoundTop,
			     double &chipBoundBot)
{
  Pin *pini, *pinj;
  double celliXPos, celliYPos, celljXPos, celljYPos;
  double piniXOffset, piniYOffset, pinjXOffset, pinjYOffset;
  double diffCellXPos, diffCellYPos, diffPinXOffPos, diffPinYOffPos;
  double totalXDiffDist, totalYDiffDist;
  double netWeight;
  uint maxx, maxy;
  
  if (newXPos == -1) celliXPos = CellGetDblX(&thisCell);
  else celliXPos = newXPos;
  if (newYPos == -1) celliYPos = CellGetDblY(&thisCell);
  else celliYPos = newYPos;

  totalXForce = 0.0;
  totalYForce = 0.0;
  magnitude = 0.0;
  forceDir = FORCE_DIR_NO_FORCE;
  chipBoundLeft = 0;
  chipBoundRight = 0;
  chipBoundTop = 0;
  chipBoundBot = 0;
  diffCellXPos = 0.0;
  diffCellYPos = 0.0;

  /* CAN CHANGE WITH NET MODEL */
  CELL_FOR_ALL_PINS_NOFILT(thisCell, PIN_DIR_ALL, pini) {
    Net &connectedNet = (*pini).PinGetNet();
    netWeight = connectedNet.NetGetWeight();
    piniXOffset = (*pini).PinGetXOffset();
    piniYOffset = (*pini).PinGetYOffset();
    NET_FOR_ALL_PINS(connectedNet, pinj) {
      Cell &connectedCell = ((*pinj).PinGetParentCell());
      if (pinj == pini) continue;
      pinjXOffset = (*pinj).PinGetXOffset();
      pinjYOffset = (*pinj).PinGetYOffset();
      diffCellXPos -= netWeight * (celliXPos + piniXOffset);
      diffCellYPos -= netWeight * (celliYPos + piniYOffset);
      if (!connectedCell.CellIsTerminal()) {
        celljXPos = CellGetDblX(&connectedCell);
        celljYPos = CellGetDblY(&connectedCell);
      } else {
        celljXPos = connectedCell.CellGetXpos();
        celljYPos = connectedCell.CellGetYpos();
      }
      diffCellXPos += netWeight * (celljXPos + pinjXOffset);
      diffCellYPos += netWeight * (celljYPos + pinjYOffset);
    } NET_END_FOR;
  } CELL_END_FOR;

  totalXForce = diffCellXPos;
  totalYForce = diffCellYPos;
  int intXForce, intYForce;
  intXForce = dtoi(totalXForce); intYForce = dtoi(totalYForce);
  if (intXForce == 0 && intYForce == 0) {
    totalXForce = intXForce;
    totalYForce = intYForce;
    return;
  }
  magnitude = sqrt((totalXForce * totalXForce + totalYForce * totalYForce));
  /* Find the equation of the line y = mx + c and its intersection 
     with the chip boundaries */
  double m, c;
  if (intXForce == 0) {
    m = 0;
    chipBoundLeft = -MAX_DBL;
    chipBoundRight = -MAX_DBL;
    chipBoundTop = maxx;
    chipBoundBot = 0.0;
    if (intYForce > 0) {
      forceDir = FORCE_DIR_TOP;
    } else if (intYForce < 0) {
      forceDir = FORCE_DIR_BOT;
    }
  } else if (intYForce == 0)  {
    chipBoundTop = -MAX_DBL;
    chipBoundBot = -MAX_DBL;
    chipBoundLeft = 0.0;
    chipBoundRight = maxy;
    if (intXForce > 0) {
      forceDir = FORCE_DIR_RIGHT;
    } else if (intXForce < 0) {
      forceDir = FORCE_DIR_LEFT;
    }
  } else {
    m = ((double)intYForce) / intXForce;
    c = (celliYPos - m * celliXPos);
    DesignGetBoundingBox(maxx, maxy);
    /* x-Coordinate of the top boundary */
    chipBoundTop = (maxy - c)/m;
    chipBoundLeft = c;
    chipBoundRight = (maxx * m) + c;
    chipBoundBot = -c / m;
    if (intXForce > 0 && intYForce > 0) {
      forceDir = FORCE_DIR_FIRST_QUAD;
    } else if (intXForce < 0 && intYForce > 0) {
      forceDir = FORCE_DIR_SECOND_QUAD;
    } else if (intXForce < 0 && intYForce < 0) {
      forceDir = FORCE_DIR_THIRD_QUAD;
    } else if (intXForce > 0 && intYForce < 0) {
      forceDir = FORCE_DIR_FOURTH_QUAD;
    }
  }
}

void
Design::DesignCreatePseudoPort(Cell &thisCell,
			       double newXpos, double newYpos,
			       double chipBoundLeft, double chipBoundRight,
			       double chipBoundTop, double chipBoundBot,
			       double magnitude, char forceDir,
			       MSKrealt *qvalijx, MSKrealt *qvalijy,
			       MSKrealt *qvalx, MSKrealt *qvaly, 
			       map<Cell *, uint> &quadMap, map<Cell *, uint> &linMap)
{
  map<Cell*, uint>::iterator quadMapItr;
  map<Cell*, uint>::iterator linMapItr;
  double spreadForce, springConstant;
  double pseudoPinX, pseudoPinY;
  double cellXpos, cellYpos;
  double portXForce, portYForce;
  double coeffX, coeffY;
  uint minx, miny, maxx, maxy;
  uint quadCellIdx, linCellIdx;
  int nodeIdx;

  DesignGetBoundingBox(maxx, maxy);
  cellXpos = newXpos;
  cellYpos = newYpos;
  minx = 0.0;
  miny = 0.0;

  switch (forceDir) {
  case FORCE_DIR_NO_FORCE: return;
  case FORCE_DIR_LEFT: pseudoPinX = maxx;
    pseudoPinY = chipBoundRight; break;
  case FORCE_DIR_RIGHT: pseudoPinX = minx;
    pseudoPinY = chipBoundLeft; break;
  case FORCE_DIR_TOP: pseudoPinY = miny;
    pseudoPinX = chipBoundBot; break;
  case FORCE_DIR_BOT: pseudoPinY = maxy;
    pseudoPinX = chipBoundTop; break;
  case FORCE_DIR_FIRST_QUAD:
    if (chipBoundLeft <= miny) {
      pseudoPinX = chipBoundBot; pseudoPinY = 0.0;
    } else if (chipBoundBot <= minx) {
      pseudoPinX = 0; pseudoPinY = chipBoundLeft;
    }
    break;
  case FORCE_DIR_SECOND_QUAD:
    if (chipBoundRight <= miny) {
      pseudoPinX = chipBoundBot; pseudoPinY = miny;
    } else if (chipBoundBot >= maxx) {
      pseudoPinX = maxx; pseudoPinY = chipBoundRight;
    }
    break;
  case FORCE_DIR_THIRD_QUAD:
    if (chipBoundTop >= maxx) {
      pseudoPinX = maxx; pseudoPinY = chipBoundRight;
    } else if (chipBoundRight >= maxy) {
      pseudoPinX = chipBoundTop; pseudoPinY = maxy;
    }
    break;
  case FORCE_DIR_FOURTH_QUAD:
    if (chipBoundLeft >= maxy) {
      pseudoPinX = chipBoundTop; pseudoPinY = maxy;
    } else if (chipBoundTop <= minx) {
      pseudoPinX = minx; pseudoPinY = chipBoundLeft;
    }
    break;
  default: cout << "DEFAULT CASE NOT EXPECTED" << endl;
  };

  /* Since we are minimizing quadratic wirelength. Model of force for 
     other objective types is going to change */
  portXForce = pseudoPinX - cellXpos;
  portYForce = pseudoPinY - cellYpos;
  spreadForce = sqrt(portXForce * portXForce + portYForce * portYForce);
  springConstant = magnitude / spreadForce;
  
  coeffX = 2 * springConstant; coeffY = coeffX;

  _KEY_EXISTS_WITH_VAL(quadMap, (&thisCell), quadMapItr) {
    quadCellIdx = quadMapItr->second;
  } else {
    cout << "SEVERE ERROR QUAD: ENTRY FOR cell: (PTR: " << &thisCell << ") " 
	 << thisCell.CellGetName() << " not found in quadMap" << endl;
  }
  _KEY_EXISTS_WITH_VAL(linMap, (&thisCell), linMapItr) {
    linCellIdx = linMapItr->second;
  } else {
    cout << "SEVERE ERROR LINEAR: ENTRY FOR cell: (PTR: " << &thisCell << ") " 
	 << thisCell.CellGetName() << " not found in linMap" << endl;
  }

  /* Update the diagonals of the quadratic matrix */
  qvalijx[quadCellIdx] += coeffX;
  qvalijy[quadCellIdx] += coeffY;

  /* Update the linear matrix */
  coeffX = coeffX * (-pseudoPinX / GRID_COMPACTION_RATIO);
  coeffY = coeffY * (-pseudoPinY / GRID_COMPACTION_RATIO);
  qvalx[linCellIdx] += (coeffX);
  qvaly[linCellIdx] += (coeffY);
}

void
spreadCellInBin(Design &myDesign, HyperGraph &myGraph, Bin *binPtr, 
		double newBinRight, double newBinTop,
		double newBinRightPrev, double newBinTopPrev, 
		double maxUtil, MSKrealt *qvalijx, MSKrealt *qvalijy, 
		MSKrealt *qvalx, MSKrealt *qvaly, 
		map<Cell *, uint> &quadMap, map<Cell *, uint> &linMap)
{
  Cell *cellPtr;
  vector<Cell *>& binCells = (*binPtr).BinGetCells();
  map<Cell *, uint>::iterator quadMapItr;
  map<Cell *, uint>::iterator linMapItr;
  uint binRight, binTop, binLeft, binBot;
  uint xj, yj;
  uint cellWidth, cellHeight;
  uint maxx, maxy;
  int quadCellIdx, linCellIdx;
  double xjPrime, yjPrime;
  double newXPos, newYPos;
  bool noXSpread, noYSpread;
  double alphaX, alphaY;
  double magnitude, totalXForce, totalYForce;
  double chipBoundLeft, chipBoundRight;
  double chipBoundTop, chipBoundBot;
  double averageCellWidth;
  uint cellLeftPos, cellBotPos, cellTopPos, cellRightPos;
  char forceDir;

  (*binPtr).BinGetBoundingBox(binLeft, binRight, binBot, binTop);
  myDesign.DesignGetBoundingBox(maxx, maxy);
  noXSpread = false; noYSpread = false;
  if (newBinRight == binRight) noXSpread = true;
  if (newBinTop == binTop) noYSpread = true;
  if (noXSpread && noYSpread) return;

  averageCellWidth = (*binPtr).BinGetAverageCellWidth();
  VECTOR_FOR_ALL_ELEMS(binCells, Cell*, cellPtr) {
    Cell &thisCell = (*cellPtr);
    thisCell.CellGetBoundingBox(cellLeftPos, cellBotPos, cellRightPos, cellTopPos);
    xj = thisCell.CellGetXpos();
    yj = thisCell.CellGetYpos();
    cellWidth = thisCell.CellGetWidth();
    cellHeight = thisCell.CellGetHeight();
    newXPos = xj; newYPos = yj;
    if (!noXSpread) {
      xjPrime = newBinRight * (xj - binLeft);
      xjPrime += newBinRightPrev * (binRight - xj);
      xjPrime /= (binRight - binLeft);
      alphaX = 0.5 + ((0.5/maxUtil) * (averageCellWidth/cellHeight));
      newXPos = xj + alphaX * (xjPrime - xj);
    }
    if (!noYSpread) {
      yjPrime = newBinTop * (yj - binBot);
      yjPrime += newBinTopPrev * (binTop - yj);
      yjPrime /= (binTop - binBot);
      alphaY = 0.8 + (0.5/maxUtil);
      newYPos = yj + alphaY * (yjPrime - yj);
    }
    //    cout <<"DBG: CELL: " << thisCell.CellGetName() << " OLD: X: " << xj <<" Y: " << yj <<" NEW: X: " << newXPos << " Y: "<< newYPos << " B: OLD R : " << binRight << " OLD T:" << binTop << " NEW R: " << newBinRight << " NEW T: " << newBinTop << endl;

    myDesign.DesignGetForceOnCell(thisCell, newXPos, newYPos, magnitude,
				  totalXForce, totalYForce, forceDir, 
				  chipBoundLeft, chipBoundRight, chipBoundTop, 
				  chipBoundBot);
    myDesign.DesignCreatePseudoPort(thisCell, newXPos, newYPos,
				    chipBoundLeft, chipBoundRight, chipBoundTop,
				    chipBoundBot, magnitude, forceDir, 
				    qvalijx, qvalijy, qvalx, qvaly, 
				    quadMap, linMap);
  } END_FOR;
}

void
Design::DesignSpreadCellsFast(HyperGraph &myGraph, MSKrealt *qvalijx, MSKrealt *qvalijy, 
			      MSKrealt *qvalx, MSKrealt *qvaly, 
			      map<Cell *, uint> &quadMap, map<Cell *, uint> &linMap)
{
  Bin *binPtr;
  vector<Cell *> cellsOfBin;
  double xiMinus1, xi, xiPlus1;
  double newXiMinus1, newYiMinus1;
  double yiMinus1, yi, yiPlus1;
  double delta, maxUtil;
  double Ui, UiPlus1, xiPrime, yiPrime;
  uint binIdx, numBins;
  int startBinIdx, nextBinIdx, prevBinIdx;
  Bin *prevBinPtr, *nextBinPtr, *curBinPtr;

  numBins = DesignBins.size();
  /* Set the value of delta = 1.5 */
  delta = 1.5; maxUtil = DesignGetPeakUtil();
  /* Assign bin idx to the start first row */
  startBinIdx = 0;
  _STEP_BEGIN("Perform row stretching");
  do {
    /* Traverse each row */
    binIdx = startBinIdx;
    do {
      curBinPtr = DesignBins[binIdx];
      xiMinus1 = 0;
      prevBinIdx = DesignGetPrevRowBinIdx(binIdx);
      nextBinIdx = DesignGetNextRowBinIdx(binIdx);
      if (prevBinIdx >= 0) {
        xiMinus1 = (*(DesignBins[prevBinIdx])).BinGetRight();
      }
      if (nextBinIdx <= 0) {
        break;
      }
      nextBinPtr = DesignBins[nextBinIdx];
      Ui = (*curBinPtr).BinGetUtilization();
      UiPlus1 = (*nextBinPtr).BinGetUtilization();
      xi = (*curBinPtr).BinGetRight();
      xiPlus1 = (*nextBinPtr).BinGetRight();
      xiPrime = (xiMinus1) * (UiPlus1 + delta) + (xiPlus1) * (Ui + delta);
      xiPrime /= (Ui + UiPlus1 + 2 * delta);
      (*curBinPtr).BinSetNewRight(xiPrime);
      binIdx = nextBinIdx;
    } while (1);
    startBinIdx = DesignGetNextColBinIdx(startBinIdx);
  } while (startBinIdx >= 0);
  _STEP_END("Perform row stretching");
  _STEP_BEGIN("Perform column stretching");
  startBinIdx = 0;
  do {
    /* Traverse each column */
    binIdx = startBinIdx;
    do {
      yiMinus1 = 0;
      prevBinIdx = DesignGetPrevColBinIdx(binIdx);
      nextBinIdx = DesignGetNextColBinIdx(binIdx);
      if (prevBinIdx >= 0) {
        yiMinus1 = (*(DesignBins[prevBinIdx])).BinGetTop();
      }
      if (nextBinIdx <= 0) {
        break;
      }
      curBinPtr = DesignBins[binIdx];
      nextBinPtr = DesignBins[nextBinIdx];
      Ui = (*curBinPtr).BinGetUtilization();
      UiPlus1 = (*nextBinPtr).BinGetUtilization();
      yi = (*curBinPtr).BinGetTop();
      yiPlus1 = (*nextBinPtr).BinGetTop();
      yiPrime = (yiMinus1) * (UiPlus1 + delta) + (yiPlus1) * (Ui + delta);
      yiPrime /= (Ui + UiPlus1 + 2 * delta);
      (*curBinPtr).BinSetNewTop(yiPrime);
      binIdx = nextBinIdx;
    } while (1);
    startBinIdx = DesignGetNextRowBinIdx(startBinIdx);
  } while (startBinIdx >= 0);
  _STEP_END("Perform column stretching");
  /* For each bin, iterate over all the cells of the bin
     and compute their new X Y position resulting from 
     spreading and create the pseudo nets and ports to 
     bring the cell to the same position */
  double prevBinNewTop, prevBinNewRight;
  double thisBinNewTop, thisBinNewRight;
  _STEP_BEGIN("Perform bin spreading");
  for (binIdx = 0; binIdx < numBins; binIdx++) {
    prevBinNewRight = 0; prevBinNewTop = 0;
    prevBinIdx = DesignGetPrevRowBinIdx(binIdx);
    if (prevBinIdx != -1) {
      prevBinNewRight = (*(DesignBins[prevBinIdx])).BinGetNewRight();
    }
    prevBinIdx = DesignGetPrevColBinIdx(binIdx);
    if (prevBinIdx != -1) {
      prevBinNewTop = (*(DesignBins[prevBinIdx])).BinGetNewTop();
    }
    curBinPtr = DesignBins[binIdx];
    thisBinNewTop = (*curBinPtr).BinGetNewTop();
    thisBinNewRight = (*curBinPtr).BinGetNewRight();
    spreadCellInBin((*this), myGraph, curBinPtr, 
		    thisBinNewRight, thisBinNewTop,
                    prevBinNewRight, prevBinNewTop, 
		    maxUtil, qvalijx, qvalijy, qvalx, 
		    qvaly, quadMap, linMap);
  }
  _STEP_END("Perform bin spreading");
}

void 
Design::DesignStretchBins(void)
{
  uint numBins, binIdx;
  int prevBinIdx, nextBinIdx;
  int startBinIdx;
  double delta, maxUtil;
  double xiMinus1, xiPlus1, xi, xiPrime;
  double yiMinus1, yiPlus1, yi, yiPrime;
  double Ui, UiPlus1;
  Bin *curBinPtr, *nextBinPtr, *prevBinPtr;

  numBins = DesignBins.size();

  /* Set the value of delta = 1.5 */
  delta = 1.5; 
  /* Assign bin idx to the start first row */
  startBinIdx = 0;
  _STEP_BEGIN("Perform row stretching");
  do {
    /* Traverse each row */
    binIdx = startBinIdx;
    do {
      xiMinus1 = 0;
      prevBinIdx = DesignGetPrevRowBinIdx(binIdx);
      nextBinIdx = DesignGetNextRowBinIdx(binIdx);
      if (prevBinIdx >= 0) {
        xiMinus1 = (*(DesignBins[prevBinIdx])).BinGetRight();
      }
      if (nextBinIdx <= 0) {
        break;
      }
      curBinPtr = DesignBins[binIdx];
      nextBinPtr = DesignBins[nextBinIdx];
      Ui = (*curBinPtr).BinGetUtilization();
      UiPlus1 = (*nextBinPtr).BinGetUtilization();
      xi = (*curBinPtr).BinGetRight();
      xiPlus1 = (*nextBinPtr).BinGetRight();
      xiPrime = (xiMinus1) * (UiPlus1 + delta) + (xiPlus1) * (Ui + delta);
      xiPrime /= (Ui + UiPlus1 + 2 * delta);
      (*curBinPtr).BinSetNewRight(xiPrime);
      binIdx = nextBinIdx;
    } while (1);
    startBinIdx = DesignGetNextColBinIdx(startBinIdx);
  } while (startBinIdx >= 0);
  _STEP_END("Perform row stretching");
  _STEP_BEGIN("Perform column stretching");
  startBinIdx = 0;
  do {
    /* Traverse each column */
    binIdx = startBinIdx;
    do {
      yiMinus1 = 0;
      prevBinIdx = DesignGetPrevColBinIdx(binIdx);
      nextBinIdx = DesignGetNextColBinIdx(binIdx);
      if (prevBinIdx >= 0) {
        yiMinus1 = (*(DesignBins[prevBinIdx])).BinGetTop();
      }
      if (nextBinIdx <= 0) {
        break;
      }
      curBinPtr = DesignBins[binIdx];
      nextBinPtr = DesignBins[nextBinIdx];
      Ui = (*curBinPtr).BinGetUtilization();
      UiPlus1 = (*nextBinPtr).BinGetUtilization();
      yi = (*curBinPtr).BinGetTop();
      yiPlus1 = (*nextBinPtr).BinGetTop();
      yiPrime = (yiMinus1) * (UiPlus1 + delta) + (yiPlus1) * (Ui + delta);
      yiPrime /= (Ui + UiPlus1 + 2 * delta);
      (*curBinPtr).BinSetNewTop(yiPrime);
      binIdx = nextBinIdx;
    } while (1);
    startBinIdx = DesignGetNextRowBinIdx(startBinIdx);
  } while (startBinIdx >= 0);
  _STEP_END("Perform column stretching");
}

void
Design::DesignSpreadCreatePseudoPort(Cell &thisCell, Bin &cellBin, 
				     double cellXpos, double cellYpos,
				     MSKrealt *qvalijx, MSKrealt *qvalijy,
				     MSKrealt *qvalx, MSKrealt *qvaly,
				     map<Cell *, uint> &quadMap, 
				     map<Cell *, uint> &linMap)
{
  double newBinTop, newBinRight;
  double newBinTopPrev, newBinRightPrev;
  double newXPos, newYPos;
  double alphaX, alphaY;
  double xj, yj, xjPrime, yjPrime;
  double totalXForce, totalYForce, magnitude;
  double chipBoundLeft, chipBoundRight, chipBoundTop, chipBoundBot;
  double maxUtil;
  int binIdx, prevBinIdx;
  uint binLeft, binRight, binBot, binTop;
  uint cellHeight;
  char forceDir;
  bool stretchInX, stretchInY;

  stretchInX = cellBin.BinStretchInX();
  stretchInY = cellBin.BinStretchInY();
  if (!stretchInX && !stretchInY) return;

  averageCellWidth = cellBin.BinGetAverageCellWidth();
  cellHeight = thisCell.CellGetHeight();

  xj = cellXpos;
  yj = cellYpos;
  newXPos = xj; newYPos = yj;
  newBinRightPrev = newBinTopPrev = 0.0;
  binIdx = cellBin.BinGetIdx();
  prevBinIdx = DesignGetPrevRowBinIdx(binIdx);
  if (prevBinIdx > 0) {
    Bin &prevBin = (*(DesignBins[prevBinIdx]));
    newBinRightPrev = prevBin.BinGetNewRight();
  } 
  prevBinIdx = DesignGetPrevColBinIdx(binIdx);
  if (prevBinIdx > 0) {
    Bin &prevBin = (*(DesignBins[prevBinIdx]));
    newBinTopPrev = prevBin.BinGetNewTop();
  } 
  
  cellBin.BinGetBoundingBox(binLeft, binBot, binRight, binTop);
  newBinRight = cellBin.BinGetNewRight();
  newBinTop = cellBin.BinGetNewTop();
  maxUtil = DesignGetPeakUtil();
  
  if (stretchInX) {
    xjPrime = newBinRight * (xj - binLeft);
    xjPrime += newBinRightPrev * (binRight - xj);
    xjPrime /= (binRight - binLeft);
    alphaX = 0.5 + ((0.5/maxUtil) * (averageCellWidth/cellHeight));
    newXPos = xj + alphaX * (xjPrime - xj);
  }
  if (stretchInY) {
    yjPrime = newBinTop * (yj - binBot);
    yjPrime += newBinTopPrev * (binTop - yj);
    yjPrime /= (binTop - binBot);
    alphaY = 0.8 + (0.5/maxUtil);
    newYPos = yj + alphaY * (yjPrime - yj);
  }
  
  /* COMPUTE FORCE ON CELL IN NEW POSITION */
  DesignGetForceOnCell(thisCell, newXPos, newYPos, magnitude,
		       totalXForce, totalYForce, forceDir, 
		       chipBoundLeft, chipBoundRight, chipBoundTop, 
		       chipBoundBot);
  
  /* COMPUTE THE PSEUDO PIN POSITION AND SPRING CONSTANT */
  DesignCreatePseudoPort(thisCell, newXPos, newYPos, chipBoundLeft, 
			 chipBoundRight, chipBoundTop, chipBoundBot,
			 magnitude, forceDir, qvalijx, qvalijy,
			 qvalx, qvaly, quadMap, linMap);
}