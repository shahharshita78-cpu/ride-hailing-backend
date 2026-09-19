import React, { useState } from 'react';
import { useNavigate, Link } from 'react-router-dom';
import api from '../api/api';
import { useStore } from '../store/store';

const Login: React.FC = () => {
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');
  
  const setUser = useStore((state) => state.setUser);
  const navigate = useNavigate();

  const handleLogin = async (e: React.FormEvent) => {
    e.preventDefault();
    setLoading(true);
    setError('');
    
    try {
      const res = await api.post('/auth/login', { email, password });
      setUser(res.data.user, res.data.token);
      navigate('/');
    } catch (err: any) {
      setError(err.response?.data || 'Login failed');
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="min-h-screen bg-uber-white flex flex-col justify-center px-6">
      <div className="w-full max-w-sm mx-auto">
        <h1 className="text-3xl font-bold mb-8">What's your email?</h1>
        {error && <div className="text-red-500 mb-4 text-sm">{error}</div>}
        <form onSubmit={handleLogin} className="space-y-4">
          <input
            type="email"
            value={email}
            onChange={(e) => setEmail(e.target.value)}
            placeholder="name@example.com"
            className="w-full px-4 py-3 bg-uber-gray rounded-lg focus:outline-none focus:ring-2 focus:ring-uber-black"
            required
          />
          <input
            type="password"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            placeholder="Password"
            className="w-full px-4 py-3 bg-uber-gray rounded-lg focus:outline-none focus:ring-2 focus:ring-uber-black"
            required
          />
          <button
            type="submit"
            disabled={loading}
            className="w-full bg-uber-black text-uber-white font-semibold py-3 rounded-lg hover:bg-uber-darkGray transition-colors flex justify-center items-center"
          >
            {loading ? 'Continuing...' : 'Continue'}
          </button>
        </form>
        <p className="mt-6 text-center text-sm text-gray-600">
          New here? <Link to="/register" className="text-uber-blue underline">Register</Link>
        </p>
      </div>
    </div>
  );
};

export default Login;
