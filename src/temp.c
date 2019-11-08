

short relocClusters(short clusterId, short max)
{
    short prevId = clusterId;
    short clusterId = getClusterValue(clusterId);
    bool lastSector = false;
    short index;

    if (++index >= max)
    {
        lastSector = true;
    }

    // Is sector used?
    if (clusterId == 0)
    {
        short clusterId = findEmptyCluster();
        if (clusterId >= 0xFF0)
            return -3;

        setClusterValue(prevId, clusterId);
    }

    // Bad cluster
    if (clusterId == 0xFF7)
        return -1;

    // Last-Of-File
    if (clusterId >= 0xFF8)
        return 0;

    // Reserved
    if (clusterId >= 0xFF0)
        return -2;

    return relocClusters(clusterId);
}

