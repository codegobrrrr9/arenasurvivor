using UnityEngine;
using UnityEngine.UI;

namespace Arena
{
    /// <summary>
    /// Builds uGUI hierarchies from code using the legacy Text component, so no
    /// TextMeshPro essentials import is needed. Used both by the editor bootstrap
    /// (hierarchy saved into the scene) and as a runtime fallback.
    /// </summary>
    public static class UIFactory
    {
        public static readonly Color PanelColor = new Color(0.05f, 0.06f, 0.09f, 0.88f);
        public static readonly Color Accent = new Color(0.2f, 0.47f, 0.96f);
        public static readonly Color TextColor = new Color(0.95f, 0.95f, 0.97f);
        public static readonly Color Muted = new Color(0.7f, 0.72f, 0.78f);

        private static Font _font;

        public static Font Font
        {
            get
            {
                if (_font == null) _font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
                return _font;
            }
        }

        public static RectTransform CreateRect(Transform parent, string name)
        {
            var go = new GameObject(name, typeof(RectTransform));
            go.layer = LayerMask.NameToLayer("UI");
            var rt = go.GetComponent<RectTransform>();
            rt.SetParent(parent, false);
            return rt;
        }

        public static RectTransform Stretch(RectTransform rt, float left = 0, float right = 0, float top = 0, float bottom = 0)
        {
            rt.anchorMin = Vector2.zero;
            rt.anchorMax = Vector2.one;
            rt.offsetMin = new Vector2(left, bottom);
            rt.offsetMax = new Vector2(-right, -top);
            return rt;
        }

        public static RectTransform Anchor(RectTransform rt, Vector2 anchor, Vector2 position, Vector2 size)
        {
            rt.anchorMin = anchor;
            rt.anchorMax = anchor;
            rt.pivot = anchor;
            rt.anchoredPosition = position;
            rt.sizeDelta = size;
            return rt;
        }

        public static Image CreatePanel(Transform parent, string name, Color color)
        {
            var rt = CreateRect(parent, name);
            Stretch(rt);
            var img = rt.gameObject.AddComponent<Image>();
            img.color = color;
            img.raycastTarget = true;
            return img;
        }

        public static Image CreateBox(Transform parent, string name, Vector2 anchor, Vector2 position, Vector2 size, Color color)
        {
            var rt = CreateRect(parent, name);
            Anchor(rt, anchor, position, size);
            var img = rt.gameObject.AddComponent<Image>();
            img.color = color;
            img.raycastTarget = false;
            return img;
        }

        public static Text CreateText(Transform parent, string name, string content, int size, Vector2 anchor, Vector2 position, Vector2 boxSize,
            TextAnchor alignment = TextAnchor.MiddleCenter, FontStyle style = FontStyle.Normal)
        {
            var rt = CreateRect(parent, name);
            Anchor(rt, anchor, position, boxSize);
            var text = rt.gameObject.AddComponent<Text>();
            text.font = Font;
            text.fontSize = size;
            text.fontStyle = style;
            text.text = content;
            text.color = TextColor;
            text.alignment = alignment;
            text.horizontalOverflow = HorizontalWrapMode.Wrap;
            text.verticalOverflow = VerticalWrapMode.Overflow;
            text.raycastTarget = false;
            var shadow = rt.gameObject.AddComponent<Shadow>();
            shadow.effectColor = new Color(0f, 0f, 0f, 0.6f);
            shadow.effectDistance = new Vector2(1.5f, -1.5f);
            return text;
        }

        public static Button CreateButton(Transform parent, string name, string label, Vector2 anchor, Vector2 position, Vector2 size, int fontSize = 30)
        {
            var rt = CreateRect(parent, name);
            Anchor(rt, anchor, position, size);
            var img = rt.gameObject.AddComponent<Image>();
            img.color = Accent;
            var button = rt.gameObject.AddComponent<Button>();
            var colors = button.colors;
            colors.normalColor = Accent;
            colors.highlightedColor = new Color(0.3f, 0.57f, 1f);
            colors.pressedColor = new Color(0.15f, 0.36f, 0.78f);
            colors.selectedColor = new Color(0.26f, 0.52f, 0.98f);
            colors.colorMultiplier = 1f;
            button.colors = colors;
            button.targetGraphic = img;

            var text = CreateText(rt, "Label", label, fontSize, new Vector2(0.5f, 0.5f), Vector2.zero, size, TextAnchor.MiddleCenter, FontStyle.Bold);
            Stretch(text.rectTransform, 8, 8, 4, 4);
            return button;
        }

        /// <summary>A horizontal bar whose fill width is driven by anchorMax.x.</summary>
        public static (Image back, Image fill) CreateBar(Transform parent, string name, Vector2 anchor, Vector2 position, Vector2 size, Color fillColor)
        {
            var back = CreateBox(parent, name, anchor, position, size, new Color(0f, 0f, 0f, 0.55f));
            var fill = CreateBox(back.transform, "Fill", Vector2.zero, Vector2.zero, Vector2.zero, fillColor);
            var rt = fill.rectTransform;
            rt.anchorMin = Vector2.zero;
            rt.anchorMax = new Vector2(1f, 1f);
            rt.offsetMin = new Vector2(3f, 3f);
            rt.offsetMax = new Vector2(-3f, -3f);
            return (back, fill);
        }

        public static void SetBarFraction(Image fill, float fraction)
        {
            if (fill == null) return;
            var rt = fill.rectTransform;
            rt.anchorMax = new Vector2(Mathf.Clamp01(fraction), 1f);
        }
    }
}
