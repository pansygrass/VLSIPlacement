# ifndef EDGE_ITER_H
# define EDGE_ITER_H

# define EDGE_FOR_ALL_NODES(Design, CellName, CellPtr) \
  { \
  map<string, Cell*> DesignCells = Design.DesignGetCells(); \
  map<string, Cell*>::iterator mapIter; \
  for (mapIter = DesignCells.begin(); mapIter != DesignCells.end(); mapIter++) { \
  CellName = mapIter->first; \
  CellPtr = mapIter->second;

# define EDGE_FOR_ALL_ADJACENT_NODES(Design, NetName, NetPtr) \
  { \
  map<string, Net*> DesignNets = Design.DesignGetNets(); \
  map<string, Net*>::iterator mapIter; \
  for (mapIter = DesignNets.begin(); mapIter != DesignNets.end(); mapIter++) { \
  NetName = (string)mapIter->first; \
  NetPtr = (Net *)mapIter->second;

# endif
