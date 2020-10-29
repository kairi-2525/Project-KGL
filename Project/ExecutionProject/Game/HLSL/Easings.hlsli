float EaseOutQuart(float x)
{
	return 1.f - pow(1 - x, 4);
}

float EaseInQuart(float x)
{
	return x * x * x * x;
}