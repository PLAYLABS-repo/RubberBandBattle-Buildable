#pragma once

#include "Engine/dependencies/include.h"
#include "Engine/Src/Image/Texture.h"
#include "Engine/Src/Image/Mapping.h"
#include "Engine/Src/Geom/Quad.h"
#include "Engine/Src/Input/InputMouse.h"

namespace Absolut
{

class Interface
{
public:

    // ============================================================
    // UI ELEMENT
    // ============================================================

    class Element
    {
    public:

        Image* texture = nullptr;
        Atlas* atlas = nullptr;

        std::string spriteName;
        Frame frame;

        float x = 0.0f;
        float y = 0.0f;

        float width = 100.0f;
        float height = 100.0f;

        // 9-slice border in source/destination pixels.
        float left = 3.0f;
        float right = 3.0f;
        float top = 3.0f;
        float bottom = 3.0f;

        bool nineSlice = false;

        bool hovered = false;
        bool pressed = false;
        bool clicked = false;

        // Tracks whether the mouse was down+inside last frame,
        // so "clicked" only fires once on the press edge instead
        // of every frame the button stays held.
        bool wasHeld = false;


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
        // ATLAS SPRITE
        // ========================================================

        bool SetSprite(
            Atlas* a,
            const char* name
        )
        {
            if (!a || !name)
                return false;

            Frame f;

            if (!a->get(name, f))
                return false;

            atlas = a;
            spriteName = name;
            frame = f;

            return true;
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
        //
        // "blocked" is true when a higher (later-created) element
        // has already claimed the mouse this frame, so elements
        // underneath don't also register hover/press/click.
        // ========================================================

        void Update(bool blocked = false)
        {
            Mouse& mouse = Mouse::Get();

            hovered = false;
            pressed = false;
            clicked = false;

            if (blocked)
            {
                wasHeld = false;
                return;
            }

            float mx = mouse.gameX;
            float my = mouse.gameY;

            bool inside =
                (mx >= x &&
                 mx <= x + width &&
                 my >= y &&
                 my <= y + height);

            if (inside)
            {
                hovered = true;

                if (mouse.isLDown)
                {
                    pressed = true;

                    // Only fire once, on the down transition -
                    // not every frame the button stays held.
                    if (!wasHeld)
                        clicked = true;
                }
            }

            wasHeld = inside && mouse.isLDown;
        }


        // ========================================================
        // DRAW
        // ========================================================

        void Draw()
        {
            if (!texture)
                return;

            // Atlas sprite.
            if (atlas && !spriteName.empty())
            {
                // Refresh frame in case atlas data changed.
                if (!atlas->get(spriteName, frame))
                    return;

                if (nineSlice)
                    DrawNineSlice(
                        frame.x,
                        frame.y,
                        frame.w,
                        frame.h
                    );
                else
                    DrawAtlasSprite();

                return;
            }

            // Plain full-texture element, still supports 9-slice.
            if (nineSlice)
            {
                DrawNineSlice(
                    0.0f,
                    0.0f,
                    (float)texture->width,
                    (float)texture->height
                );

                return;
            }

            DrawNormal();
        }


        // ========================================================
        // DRAW NORMAL FULL IMAGE
        // ========================================================

        void DrawNormal()
        {
            if (texture->width <= 0 ||
                texture->height <= 0)
                return;

            Quad q;

            q.texture = texture->Texture;

            q.x = x;
            q.y = y;
            q.w = width;
            q.h = height;

            q.u0 = 0.0f;
            q.v0 = 0.0f;
            q.u1 = 1.0f;
            q.v1 = 1.0f;

            q.AnchorTo(
                SCREEN,
                SCREEN
            );

            q.r = 1.0f;
            q.g = 1.0f;
            q.b = 1.0f;
            q.a = 1.0f;

            q.draw();
        }


        // ========================================================
        // DRAW ATLAS SPRITE
        //
        // Same UV mapping used by Animator::DrawSprite().
        // ========================================================

        void DrawAtlasSprite()
        {
            if (texture->width <= 0 ||
                texture->height <= 0)
                return;

            Quad q;

            q.texture = texture->Texture;

            q.x = x;
            q.y = y;

            q.w = width;
            q.h = height;

            q.AnchorTo(
                SCREEN,
                SCREEN
            );

            q.u0 =
                frame.x /
                (float)texture->width;

            q.v0 =
                frame.y /
                (float)texture->height;

            q.u1 =
                (frame.x + frame.w) /
                (float)texture->width;

            q.v1 =
                (frame.y + frame.h) /
                (float)texture->height;

            q.r = 1.0f;
            q.g = 1.0f;
            q.b = 1.0f;
            q.a = 1.0f;

            q.draw();
        }


        // ========================================================
        // DRAW 9-SLICE
        //
        // fx/fy/fw/fh describe the source rect to slice - either
        // an atlas frame, or the whole texture (0,0,w,h) when
        // there's no atlas.
        // ========================================================

        void DrawNineSlice(
            float fx,
            float fy,
            float fw,
            float fh
        )
        {
            if (texture->width <= 0 ||
                texture->height <= 0)
                return;

            float texW =
                (float)texture->width;

            float texH =
                (float)texture->height;

            float srcW = fw;
            float srcH = fh;

            if (srcW <= 0.0f ||
                srcH <= 0.0f)
                return;


            // ----------------------------------------------------
            // SOURCE BORDER
            // ----------------------------------------------------

            float srcL = left;
            float srcR = right;
            float srcT = top;
            float srcB = bottom;


            if (srcL < 0.0f) srcL = 0.0f;
            if (srcR < 0.0f) srcR = 0.0f;
            if (srcT < 0.0f) srcT = 0.0f;
            if (srcB < 0.0f) srcB = 0.0f;


            if (srcL + srcR > srcW)
            {
                float scale =
                    srcW /
                    (srcL + srcR);

                srcL *= scale;
                srcR *= scale;
            }


            if (srcT + srcB > srcH)
            {
                float scale =
                    srcH /
                    (srcT + srcB);

                srcT *= scale;
                srcB *= scale;
            }


            // ----------------------------------------------------
            // DESTINATION BORDER
            // ----------------------------------------------------

            float dstL = left;
            float dstR = right;
            float dstT = top;
            float dstB = bottom;


            // Same clamp as the source border - previously missing,
            // which let a negative border invert/garble the quads.
            if (dstL < 0.0f) dstL = 0.0f;
            if (dstR < 0.0f) dstR = 0.0f;
            if (dstT < 0.0f) dstT = 0.0f;
            if (dstB < 0.0f) dstB = 0.0f;


            if (dstL + dstR > width)
            {
                float scale =
                    width /
                    (dstL + dstR);

                dstL *= scale;
                dstR *= scale;
            }


            if (dstT + dstB > height)
            {
                float scale =
                    height /
                    (dstT + dstB);

                dstT *= scale;
                dstB *= scale;
            }


            // ----------------------------------------------------
            // DESTINATION X
            // ----------------------------------------------------

            float dx0 = x;
            float dx1 = x + dstL;
            float dx2 = x + width - dstR;
            float dx3 = x + width;


            // ----------------------------------------------------
            // DESTINATION Y
            // ----------------------------------------------------

            float dy0 = y;
            float dy1 = y + dstT;
            float dy2 = y + height - dstB;
            float dy3 = y + height;


            // ----------------------------------------------------
            // SOURCE X
            // ----------------------------------------------------

            float sx0 = fx;
            float sx1 = fx + srcL;
            float sx2 = fx + srcW - srcR;
            float sx3 = fx + srcW;


            // ----------------------------------------------------
            // SOURCE Y
            // ----------------------------------------------------

            float sy0 = fy;
            float sy1 = fy + srcT;
            float sy2 = fy + srcH - srcB;
            float sy3 = fy + srcH;


            // ----------------------------------------------------
            // SOURCE PIXELS -> UV
            // ----------------------------------------------------

            float u0 = sx0 / texW;
            float u1 = sx1 / texW;
            float u2 = sx2 / texW;
            float u3 = sx3 / texW;

            float v0 = sy0 / texH;
            float v1 = sy1 / texH;
            float v2 = sy2 / texH;
            float v3 = sy3 / texH;


            // 1 - TOP LEFT
            DrawPart(dx0, dy0, dx1 - dx0, dy1 - dy0, u0, v0, u1, v1);

            // 2 - TOP
            DrawPart(dx1, dy0, dx2 - dx1, dy1 - dy0, u1, v0, u2, v1);

            // 3 - TOP RIGHT
            DrawPart(dx2, dy0, dx3 - dx2, dy1 - dy0, u2, v0, u3, v1);

            // 4 - LEFT
            DrawPart(dx0, dy1, dx1 - dx0, dy2 - dy1, u0, v1, u1, v2);

            // 5 - CENTER
            DrawPart(dx1, dy1, dx2 - dx1, dy2 - dy1, u1, v1, u2, v2);

            // 6 - RIGHT
            DrawPart(dx2, dy1, dx3 - dx2, dy2 - dy1, u2, v1, u3, v2);

            // 7 - BOTTOM LEFT
            DrawPart(dx0, dy2, dx1 - dx0, dy3 - dy2, u0, v2, u1, v3);

            // 8 - BOTTOM
            DrawPart(dx1, dy2, dx2 - dx1, dy3 - dy2, u1, v2, u2, v3);

            // 9 - BOTTOM RIGHT
            DrawPart(dx2, dy2, dx3 - dx2, dy3 - dy2, u2, v2, u3, v3);
        }


        // ========================================================
        // DRAW ONE 9-SLICE PART
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
            if (pw <= 0.0f ||
                ph <= 0.0f)
                return;

            Quad q;

            q.texture =
                texture->Texture;

            q.x = px;
            q.y = py;

            q.w = pw;
            q.h = ph;

            q.u0 = uu0;
            q.v0 = vv0;

            q.u1 = uu1;
            q.v1 = vv1;

            q.AnchorTo(
                SCREEN,
                SCREEN
            );

            q.r = 1.0f;
            q.g = 1.0f;
            q.b = 1.0f;
            q.a = 1.0f;

            q.draw();
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
        Element* element =
            new Element();

        elements.push_back(element);

        return *element;
    }


    // ============================================================
    // UPDATE
    //
    // Iterates back-to-front (topmost / last-created element
    // first) so only the element actually on top under the
    // cursor registers hover/press/click - elements behind it
    // no longer get "clicked through".
    // ============================================================

    void Update()
    {
        bool consumed = false;

        for (int i = (int)elements.size() - 1;
             i >= 0;
             --i)
        {
            if (!elements[i])
                continue;

            elements[i]->Update(consumed);

            if (elements[i]->hovered)
                consumed = true;
        }
    }


    // ============================================================
    // DRAW
    // ============================================================

    void Draw()
    {
        for (size_t i = 0;
             i < elements.size();
             ++i)
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
        for (size_t i = 0;
             i < elements.size();
             ++i)
        {
            delete elements[i];
        }

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
