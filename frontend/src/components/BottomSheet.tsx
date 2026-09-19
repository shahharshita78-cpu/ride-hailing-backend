import React from 'react';

interface BottomSheetProps {
  children: React.ReactNode;
}

const BottomSheet: React.FC<BottomSheetProps> = ({ children }) => {
  return (
    <div className="absolute bottom-0 left-0 right-0 bg-uber-white rounded-t-3xl shadow-[0_-4px_20px_rgba(0,0,0,0.1)] p-6 pb-8 z-50 transition-transform duration-300 transform translate-y-0">
      <div className="w-12 h-1 bg-gray-300 rounded-full mx-auto mb-6"></div>
      {children}
    </div>
  );
};

export default BottomSheet;
