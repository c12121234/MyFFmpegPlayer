#ifndef IVIDEOCALL_H
#define IVIDEOCALL_H

struct AVFrame;
class IVideoCall
{
public:
    virtual void Init(int nWidth, int nHeight) = 0;
    virtual void Repaint(AVFrame* pFrame) = 0;
};


#endif // IVIDEOCALL_H
