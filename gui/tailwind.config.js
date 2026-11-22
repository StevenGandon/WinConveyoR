/** @type {import('tailwindcss').Config} */
export default {
  content: ['./index.html', './src/**/*.{js,ts,jsx,tsx}'],
  darkMode: 'class',
  theme: {
    extend: {
      fontFamily: {
        sans: ['"IBM Plex Sans"', '"Segoe UI"', 'system-ui', 'sans-serif'],
        mono: ['"IBM Plex Mono"', 'Cascadia Code', 'Consolas', 'monospace'],
      },
      colors: {
        wc: {
          accent: "#E8913A",
          "accent-deep": "#CE7A24",
          "accent-soft": "#F7E3CB",
          "accent-soft-dark": "#352A1C",
          "accent-bright": "#F0A152",

          bg: "#FFF7EE",
          card: "#FFFDF9",
          sidebar: "#F4EADB",
          surface: "#EADDC8",
          border: "#E7D8C3",
          "border-strong": "#D3BFA9",
          muted: "#877969",
          fg: "#1D1D1F",

          "bg-dark": "#1E1E1E",
          "card-dark": "#252526",
          "sidebar-dark": "#202020",
          "surface-dark": "#2E2E31",
          "border-dark": "#393939",
          "border-strong-dark": "#4A4A4D",
          "muted-dark": "#A99A8A",
          "fg-dark": "#E9E3DA",

          success: "#3E7A4D",
          "success-soft": "#E2ECDD",
          "success-dark": "#5FB57A",
          "success-soft-dark": "#26301F",
          danger: "#C0492F",
          "danger-soft": "#F4DED7",
          "danger-dark": "#E2704F",
          "danger-soft-dark": "#3A211B",
        },
      },
    },
  },
  plugins: [],
};
