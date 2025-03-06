mat3 rotationYMatrix(float angle) 
{
	return mat3(cos(angle),  0.0, sin(angle),
				0.0,         1.0, 0.0,      
				-sin(angle), 0.0, cos(angle));
}

mat3 rotationZMatrix(float angle)
{
	return mat3(cos(angle), -sin(angle), 0.0,
	            sin(angle), cos(angle),  0.0,
				0.0,        0.0,         1.0);
}

mat3 rotationXMatrix(float angle)
{
	return mat3(1.0, 0.0,        0.0,
	            0.0, cos(angle), -sin(angle),
				0.0, sin(angle), cos(angle));
}

mat4 translationMatrix(vec3 translate)
{
	return mat4(1.0, 0.0, 0.0, translate.x,
	            0.0, 1.0, 0.0, translate.y,
				0.0, 0.0, 1.0, translate.z,
				0.0, 0.0, 0.0, 1.0);
}
