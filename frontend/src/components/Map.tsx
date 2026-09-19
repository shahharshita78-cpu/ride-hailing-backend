import React, { useEffect, useRef } from 'react';
import { MapContainer, TileLayer, Marker, useMap } from 'react-leaflet';
import L from 'leaflet';
import 'leaflet/dist/leaflet.css';

// Fix Leaflet's default icon path issues with Webpack/Vite
delete (L.Icon.Default.prototype as any)._getIconUrl;
L.Icon.Default.mergeOptions({
  iconRetinaUrl: 'https://unpkg.com/leaflet@1.9.4/dist/images/marker-icon-2x.png',
  iconUrl: 'https://unpkg.com/leaflet@1.9.4/dist/images/marker-icon.png',
  shadowUrl: 'https://unpkg.com/leaflet@1.9.4/dist/images/marker-shadow.png',
});

interface MapProps {
  center: [number, number];
  driverLocation?: [number, number] | null;
  className?: string;
}

const Recenter = ({ center }: { center: [number, number] }) => {
  const map = useMap();
  useEffect(() => {
    map.flyTo(center, 15, { animate: true, duration: 1.5 });
  }, [center, map]);
  return null;
};

const CustomMap: React.FC<MapProps> = ({ center, driverLocation, className }) => {
  return (
    <div className={`h-full w-full ${className || ''}`}>
      <MapContainer
        center={center}
        zoom={15}
        zoomControl={false}
        className="h-full w-full"
      >
        <TileLayer
          attribution='&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a>'
          url="https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png" // Dark theme
        />
        <Recenter center={center} />
        
        {/* User Location */}
        <Marker position={center} />

        {/* Driver Location */}
        {driverLocation && (
          <Marker 
            position={driverLocation} 
            icon={new L.Icon({
              iconUrl: 'https://cdn-icons-png.flaticon.com/512/3204/3204996.png', // Temporary car icon
              iconSize: [32, 32],
              iconAnchor: [16, 16]
            })}
          />
        )}
      </MapContainer>
    </div>
  );
};

export default CustomMap;
