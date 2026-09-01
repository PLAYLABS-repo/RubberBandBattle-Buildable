#pragma once

#include "Engine/dependencies/include.h"
#include "Engine/Src/Image/Texture.h"
#include "Engine/Src/Geom/Quad.h"
#include "Engine/Src/Input/InputMouse.h"

namespace Absolut
{

class Interface
{
public:

    // ============================================================
    // 9-SLICE ELEMENT
    //
    //       1 | 2 | 3
    //      ---+---+---
    //       4 | 5 | 6
    //      ---+---+---
    //       7 | 8 | 9
    //
    // Corners do NOT stretch.
    // Edges stretch in one direction.
    // Center stretches in both directions.
    // ============================================================

    class Element
    {
    public:

        Image* texture = nullptr;

        float x = 0.0f;
        float y = 0.0f;

        float width = 100.0f;
        float height = 100.0f;

        // Size of the borders in pixels
        float left = 8.0f;
        float right = 8.0f;
        float top = 8.0f;
        float bottom = 8.0f;

        bool nineSlice = false;

        bool hovered = false;
        bool pressed = false;
        bool clicked = false;


        // ========================================================
        // POSITION
        // ========================================================

        void SetPosition(float px, float py)
        {
            x = px;
            y = py;
        }


        // ========================================================
        // SIZE
        // ========================================================

        void SetSize(float w, float h)
        {
            width = w;
            height = h;
        }


        // ========================================================
        // TEXTURE
        // ========================================================

        void SetTexture(Image* img)
        {
            texture = img;
        }


        // ========================================================
        // ENABLE 9-SLICE
        // ========================================================

        void SetNineSlice(
            float l,
            float r,
            float t,
            float b
        )
        {
            left = l;
            right = r;
            top = t;
            bottom = b;

            nineSlice = true;
        }


        // ========================================================
        // DISABLE 9-SLICE
        // ========================================================

        void DisableNineSlice()
        {
            nineSlice = false;
        }


        // ========================================================
        // INPUT
        // ========================================================

        void Update()
        {
            Mouse& mouse = Mouse::Get();

            hovered = false;
            pressed = false;
            clicked = false;

            float mx = mouse.gameX;
            float my = mouse.gameY;

            if (mx >= x &&
                mx <= x + width &&
                my >= y &&
                my <= y + height)
            {
                hovered = true;

                if (mouse.isLDown)
                    pressed = true;

                if (mouse.isLDown)
                    clicked = true;
            }
        }


        // ========================================================
        // DRAW
        // ========================================================

        void Draw()
        {
            if (!texture)
                return;

            if (nineSlice)
                DrawNineSlice();
            else
                DrawNormal();
        }


        // ========================================================
        // NORMAL IMAGE
        // ========================================================

        void DrawNormal()
        {
            Quad quad;

            quad.AnchorTo(SCREEN, SCREEN);

            quad.x = x;
            quad.y = y;

            quad.w = width;
            quad.h = height;

            quad.u0 = 0.0f;
            quad.v0 = 0.0f;

            quad.u1 = 1.0f;
            quad.v1 = 1.0f;

            quad.texture = texture->Texture;

            quad.draw();
        }


        // ========================================================
        // 9-SLICE
        // ========================================================

        void DrawNineSlice()
        {
            float texW = (float)texture->width;
            float texH = (float)texture->height;

            if (texW <= 0.0f || texH <= 0.0f)
                return;


            // ----------------------------------------------------
            // Prevent the borders from becoming larger than the
            // actual element.
            // ----------------------------------------------------

            float L = left;
            float R = right;
            float T = top;
            float B = bottom;

            if (L + R > width)
            {
                float scale = width / (L + R);

                L *= scale;
                R *= scale;
            }

            if (T + B > height)
            {
                float scale = height / (T + B);

                T *= scale;
                B *= scale;
            }


            // ----------------------------------------------------
            // X positions
            // ----------------------------------------------------

            float x0 = x;
            float x1 = x + L;
            float x2 = x + width - R;
            float x3 = x + width;


            // ----------------------------------------------------
            // Y positions
            // ----------------------------------------------------

            float y0 = y;
            float y1 = y + T;
            float y2 = y + height - B;
            float y3 = y + height;


            // ----------------------------------------------------
            // UV positions
            // ----------------------------------------------------

            float u0 = 0.0f;
            float u1 = L / texW;
            float u2 = 1.0f - (R / texW);
            float u3 = 1.0f;


            float v0 = 0.0f;
            float v1 = T / texH;
            float v2 = 1.0f - (B / texH);
            float v3 = 1.0f;


            // ====================================================
            // TOP
            // ====================================================

            DrawPart(
                x0, y0,
                L, T,
                u0, v0,
                u1, v1
            );

            DrawPart(
                x1, y0,
                x2 - x1, T,
                u1, v0,
                u2, v1
            );

            DrawPart(
                x2, y0,
                R, T,
                u2, v0,
                u3, v1
            );


            // ====================================================
            // MIDDLE
            // ====================================================

            DrawPart(
                x0, y1,
                L, y2 - y1,
                u0, v1,
                u1, v2
            );

            DrawPart(
                x1, y1,
                x2 - x1,
                y2 - y1,
                u1, v1,
                u2, v2
            );

            DrawPart(
                x2, y1,
                R, y2 - y1,
                u2, v1,
                u3, v2
            );


            // ====================================================
            // BOTTOM
            // ====================================================

            DrawPart(
                x0, y2,
                L, B,
                u0, v2,
                u1, v3
            );

            DrawPart(
                x1, y2,
                x2 - x1, B,
                u1, v2,
                u2, v3
            );

            DrawPart(
                x2, y2,
                R, B,
                u2, v2,
                u3, v3
            );
        }


        // ========================================================
        // DRAW ONE SLICE
        // ========================================================

        void DrawPart(
            float px,
            float py,
            float pw,
            float ph,

            float uu0,
            float vv0,
            float uu1,
            float vv1
        )
        {
            if (pw <= 0.0f || ph <= 0.0f)
                return;

            Quad quad;

            quad.AnchorTo(SCREEN, SCREEN);

            quad.x = px;
            quad.y = py;

            quad.w = pw;
            quad.h = ph;

            quad.u0 = uu0;
            quad.v0 = vv0;

            quad.u1 = uu1;
            quad.v1 = vv1;

            quad.texture = texture->Texture;

            quad.draw();
        }


        // ========================================================
        // STATE
        // ========================================================

        bool IsHovered() const
        {
            return hovered;
        }

        bool IsPressed() const
        {
            return pressed;
        }

        bool IsClicked() const
        {
            return clicked;
        }
    };


    // ============================================================
    // CREATE ELEMENT
    // ============================================================

    Element& CreateElement()
    {
        Element* element = new Element();

        elements.push_back(element);

        return *element;
    }


    // ============================================================
    // UPDATE
    // ============================================================

    void Update()
    {
        for (size_t i = 0; i < elements.size(); ++i)
        {
            if (elements[i])
                elements[i]->Update();
        }
    }


    // ============================================================
    // DRAW
    // ============================================================

    void Draw()
    {
        for (size_t i = 0; i < elements.size(); ++i)
        {
            if (elements[i])
                elements[i]->Draw();
        }
    }


    // ============================================================
    // CLEAR
    // ============================================================

    void Clear()
    {
        for (size_t i = 0; i < elements.size(); ++i)
            delete elements[i];

        elements.clear();
    }


    // ============================================================
    // DESTRUCTOR
    // ============================================================

    ~Interface()
    {
        Clear();
    }


private:

    std::vector<Element*> elements;
};

}
