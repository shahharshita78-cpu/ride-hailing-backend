/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {
      colors: {
        uber: {
          black: '#000000',
          white: '#ffffff',
          gray: '#f3f3f3',
          darkGray: '#222222',
          yellow: '#f4c300',
          blue: '#276ef1',
        }
      },
      fontFamily: {
        sans: ['Inter', 'system-ui', 'sans-serif'],
      }
    },
  },
  plugins: [],
}
