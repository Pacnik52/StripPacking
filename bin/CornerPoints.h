#pragma once
#include "P2D.h"
#include "BinpackData.h"
#include <vector>

namespace binpack
{

    struct InsertionPoint
    {
        int idx;
        P2D P1;
        bool Rotated;

        InsertionPoint() : P1(), Rotated(false) {}

        InsertionPoint( P2D _P1, bool _Rotated )
            :P1(_P1), Rotated(_Rotated)
        {
        }
        InsertionPoint( int X, int Y, bool _Rotated)
            : P1(X,Y), Rotated(_Rotated)
        {
        }
    };

    struct InsertionQuality
    {
        int VolumeIncrease = 0;

        int MismatchX = 0, MismatchY = 0;
        bool Infeasible = false; // insertion infeasible

        int DX = 0, DY = 0; //< box's distance from edges of the palette
    };


    typedef pair<InsertionPoint, InsertionQuality> EvaListElementType;
    typedef vector<EvaListElementType> EvaListType;


    struct CornerPoints
    {
        int Area; //< area determined by corner points

        vector<P2D> CPList; //< corner point list


        int FindStartPos(int X);
        void insertCP(int CPIdx, int SizeX, int SizeY, bool Insert, int& AreaDiff, int &MismatchX, int &MismatchY);
        void insertCP(int CPIdx, int SizeX, int SizeY);

        void ComputeArea();

        int PSizeX, PSizeY; //< Palette dimensions

        CornerPoints() :PSizeX(-1), PSizeY(-1) {

        }
        CornerPoints(int _PSizeX, int _PSizeY);

        void Evaluate(const BinpackData::BoxType *Box,
            bool Rotated,
            EvaListType& EvaList);

        void Insert(const BinpackData::BoxType* Box, const pair<InsertionPoint, InsertionQuality>& IPIQ);

    private:
        bool Evaluate(const BinpackData::BoxType* Box,
            InsertionPoint IP,
            InsertionQuality &IQ);
    };


}
