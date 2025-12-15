#include "CornerPoints.h"

#include <cassert>
#include <random>

namespace binpack {

    CornerPoints::CornerPoints(int _PSizeX, int _PSizeY ) : PSizeX(_PSizeX), PSizeY(_PSizeY)
    {
        Area = 0;

        CPList.emplace_back(0, 0);
    }


    void CornerPoints::Evaluate(const BinpackData::BoxType *Box,
            bool Rotated,
            EvaListType& EvaList)
    {
        InsertionPoint IP;

        IP.Rotated = Rotated;

        for (int idx = 0; idx < CPList.size(); idx++) {

            IP.idx = idx;
            IP.P1 = CPList[idx];

            InsertionQuality IQ;
           
            if (Evaluate(Box, IP, IQ))
            {
                EvaList.emplace_back(IP, IQ);
            }
        }
    }

    bool CornerPoints::Evaluate(const BinpackData::BoxType *Box,
            InsertionPoint IP,
            InsertionQuality &IQ)
    // ocenia wstawienie Box'a w InsertionPoint
    // orientacja pude�ka jest taka, jak podana w IP
    // wynik zapisuje w IQ
    // zwraca false je�li nie da si� wstawi� z powodu przekroczenia r�nych ogranicze�
    {
        IQ.Infeasible = true;
        IQ.MismatchX = 0; IQ.MismatchY = 0;

        // ustalenie rozmiar�w box'a z uwzgl obrotu
        int SizeX, SizeY;

        if (IP.Rotated) {
            SizeX = Box->SizeY;
            SizeY = Box->SizeX;
        } else {
            SizeX = Box->SizeX;
            SizeY = Box->SizeY;

        }

        // prawy przedni rog box'a
        P2D P2(IP.P1.X + SizeX - 1, IP.P1.Y + SizeY - 1);

        // czy miesci sie na palecie
        if (P2.X >= PSizeX || P2.Y >= PSizeY) {
            return false;
        }


        insertCP(IP.idx, SizeX, SizeY, false, IQ.VolumeIncrease, IQ.MismatchX, IQ.MismatchY);

        IQ.Infeasible = false;
        IQ.DX = PSizeX - (P2.X + 1);
        IQ.DY = PSizeY - (P2.Y + 1);
        return true;
    }

    void CornerPoints::Insert(const BinpackData::BoxType *Box, const pair<InsertionPoint, InsertionQuality>& IPIQ)
    // wstawia pude�ko na pozycji InsertionPoint
    // assert(Box.SizeZ > 0)
    {
        InsertionPoint IP = IPIQ.first;
        const InsertionQuality &IQ = IPIQ.second;

        int SizeX = IP.Rotated ? Box->SizeY : Box->SizeX;
        int SizeY = IP.Rotated ? Box->SizeX : Box->SizeY;

        insertCP(IP.idx, SizeX, SizeY);
    }



    void  CornerPoints::insertCP(int idx, int SizeX, int SizeY, bool Insert, int& AreaDiff, int &MismatchX, int& MismatchY)
        // P1, P2 sa to wsporzedne lewego-tylnego i prawego-przedniego rogu wstawianego Box'a
        // Jesli P2 jest niezdominowany i Insert == true, to jest wstawiany do listy NDP
        // i s� usuwane punkty przez niego dominowane, przeliczana jest te� nowa powierzchnia warstwy;
        // Je�li P2 jest niezdominowany to przeliczane s� parametry AreaDiff i HorizontalFit
        // zwraca false wtw, gdy P2 jest zdominowany
    {
        //             int AreaTmp = 0;
        AreaDiff = 0;

        P2D P2(CPList[idx].X+ SizeX, CPList[idx].Y+ SizeY);

        assert( idx < CPList.size());

        if (idx < CPList.size()-1) {
            MismatchX = P2.X - (CPList[idx+1].X);
        } else {
            MismatchX = 0;
        }
        if (idx > 0) {
            MismatchY = P2.Y - (CPList[idx-1].Y);
        } else {
            MismatchY = 0;
        }

        AreaDiff += SizeX * SizeY;

        int beg;
        for( beg = idx - 1; beg >= 0 && CPList[beg].Y <= P2.Y; beg-- ) {
            AreaDiff += (CPList[beg+1].X - CPList[beg].X) * (P2.Y - CPList[beg].Y);
        }
        beg += 1;

        int end;
        for( end = idx + 1; end < CPList.size() && CPList[end].X <= P2.X; end++ ) {
            AreaDiff += (CPList[end-1].Y - CPList[end].Y) * (P2.X - CPList[end].X);
        }
        end -= 1;


        if (Insert) {
            int numRemoved = end - beg + 1;
            int X = (beg > 0 ? CPList[beg].X : 0);
            P2D L(X, P2.Y);
            int Y = (end < CPList.size() ? CPList[end].Y : 0);
            P2D R(P2.X, Y);
            if (numRemoved >= 2) {
                CPList.erase(CPList.begin() + beg, CPList.begin() + beg + (numRemoved-2));
                CPList[beg] = L;
            } else {
                CPList.insert(CPList.begin() + beg, L);
            }
            CPList[beg+1] = R;
        }
    }

    void CornerPoints::insertCP(int CPIdx, int SizeX, int SizeY)
            // j.w. ale z parametrem Insert = true
        {
        int Tmp1, Tmp2, Tmp3;
        return insertCP(CPIdx, SizeX, SizeY, true, Tmp1, Tmp2, Tmp3);
        }


    void  CornerPoints::ComputeArea()
        // Przelicza powierzchni� poziomu
        // powierzchnia jest wyznaczana przez punkty niezdomiowane
    {
        int ltx = 0;
        Area = 0;

        for (int beg = 1; beg < CPList.size(); beg++)
        {
            Area += CPList[beg-1].Y * (CPList[beg].X - ltx);
            ltx = CPList[beg].X;
        }
    }


}