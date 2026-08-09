#include "Quad.h"

namespace Absolut {
   void Quad::draw()
    {
          bool hasTexture = (texture != 0);

        if (hasTexture)
        {

               glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        }

        glPushMatrix();



        float sx = tanf(SkewX * 3.14159265f / 180.0f);
        float sy = tanf(SkewY * 3.14159265f / 180.0f);

        float matrix[16] = {
            1.0f, -sy,  0.0f, 0.0f,
            -sx,  1.0f, 0.0f, 0.0f,
            0.0f,  0.0f, 1.0f, 0.0f,
            0.0f,  0.0f, 0.0f, 1.0f
        };

        glMultMatrixf(matrix);
 glTranslatef(x, y, 0.0f);



    glTranslatef(PivotX, PivotY, 0.0f);

    glRotatef(
        Rotation,
        0.0f,
        0.0f,
        1.0f
    );

    glTranslatef(-PivotX, -PivotY, 0.0f);

    glTranslatef(
        BitmapOffsetX,
        BitmapOffsetY,
        0.0f
    );


        glBegin(GL_QUADS);

          glColor3f(r, g, b);

    glTexCoord2f(u0, v0);
    glVertex2f(0.0f, 0.0f);

    glTexCoord2f(u1, v0);
    glVertex2f(w, 0.0f);

    glTexCoord2f(u1, v1);
    glVertex2f(w, h);

    glTexCoord2f(u0, v1);
    glVertex2f(0.0f, h);
        glEnd();

        glPopMatrix();
        if (hasTexture)
        {
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_TEXTURE_2D);
        }
    }

}
