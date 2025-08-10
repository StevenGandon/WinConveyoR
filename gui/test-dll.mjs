import { load, DataType } from 'ffi-rs';
import path from 'path';
import fs from 'fs';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const libPath = path.join(__dirname, 'src', 'lib', 'testlib.dll');

console.log('=== Diagnostic de la DLL ===');
console.log('Chemin:', libPath);
console.log('Fichier existe:', fs.existsSync(libPath));

if (fs.existsSync(libPath)) {
  const stats = fs.statSync(libPath);
  console.log('Taille:', stats.size, 'bytes');
  console.log('Modifié le:', stats.mtime);
  
  console.log('\n=== Test de chargement des fonctions ===');
  
  // Test fonction add
  try {
    console.log('Test de add(5, 3)...');
    const result1 = load({
      library: libPath,
      funcName: 'add',
      retType: DataType.I32,
      paramsType: [DataType.I32, DataType.I32],
      paramsValue: [5, 3]
    });
    console.log('✓ add(5, 3) =', result1);
  } catch (error) {
    console.log('✗ Erreur add:', error.message);
    console.log('Stack trace:', error.stack);
    
    // Essayer avec différentes calling conventions
    try {
      console.log('Essai avec stdcall...');
      const result1_stdcall = load({
        library: libPath,
        funcName: 'add',
        retType: DataType.I32,
        paramsType: [DataType.I32, DataType.I32],
        paramsValue: [5, 3],
        callingConvention: 'stdcall'
      });
      console.log('✓ add(5, 3) avec stdcall =', result1_stdcall);
    } catch (error2) {
      console.log('✗ Erreur add avec stdcall:', error2.message);
    }
  }
  
  // Test fonction sub
  try {
    console.log('\nTest de sub(10, 4)...');
    const result2 = load({
      library: libPath,
      funcName: 'sub',
      retType: DataType.I32,
      paramsType: [DataType.I32, DataType.I32],
      paramsValue: [10, 4]
    });
    console.log('✓ sub(10, 4) =', result2);
  } catch (error) {
    console.log('✗ Erreur sub:', error.message);
    console.log('Stack trace:', error.stack);
  }
} else {
  console.log('✗ Le fichier DLL n\'existe pas!');
}

console.log('\n=== Informations système ===');
console.log('Platform:', process.platform);
console.log('Architecture:', process.arch);
console.log('Node version:', process.version);