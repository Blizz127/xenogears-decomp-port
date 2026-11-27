int FieldMathInterpolateAngle(int from, int to, int delta)
{
    unsigned int start = (unsigned int)from & 0xfffu;
    unsigned int target = (unsigned int)to & 0xfffu;
    unsigned int distance;
    unsigned int stepped;
    int forward;

    distance = (target - start) & 0xfffu;
    forward = distance != 0 && distance <= 0x800u;
    if (forward)
        stepped = (start + (unsigned int)delta) & 0xfffu;
    else
        stepped = (start - (unsigned int)delta) & 0xfffu;

    distance = (target - stepped) & 0xfffu;
    if (forward != (distance != 0 && distance <= 0x800u))
        return (int)target;
    return (int)stepped;
}

int FieldMathUpdateAngle(int curAngle, int targetAngle, int delta)
{
    if (D_800ADC18 != 0)
        return (int)((unsigned int)targetAngle & 0xfffu);
    return FieldMathInterpolateAngle(curAngle, targetAngle, delta);
}
