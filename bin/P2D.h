#pragma once


namespace binpack
{
    struct P2D // Point 2D
    {
        int X, Y;
        P2D() : X(0), Y(0) {};
        P2D(int _X, int _Y)
        {
            X = (int)_X;
            Y = (int)_Y;
        }
    };

}
