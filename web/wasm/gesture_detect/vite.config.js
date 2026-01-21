// vite.config.js
import { defineConfig } from 'vite'
import { execSync } from 'child_process'
import fs from 'fs'
import path from 'path'

export default defineConfig({
    plugins: [
        {
            name: 'make-and-copy',
            buildStart() {
                // Run make before build
                try {
                    console.log('Running make...')
                    execSync('make', { stdio: 'inherit' })
                } catch (error) {
                    console.error('Make failed:', error.message)
                }
            },
            closeBundle() {
                // Copy build directory after vite build
                const buildDir = './build'
                const destDir = './dist/build'

                if (fs.existsSync(buildDir)) {
                    console.log('Copying build directory...')
                    copyDir(buildDir, destDir)
                }
            }
        }
    ],
    // Optionally include build directory as static assets
    publicDir: false, // Disable default public dir if needed
    build: {
        // Copy build files to dist
        assetsDir: '.', // Put assets in root of dist
        // or use rollupOptions to copy files
    }
})

function copyDir(src, dest) {
    if (!fs.existsSync(dest)) {
        fs.mkdirSync(dest, { recursive: true })
    }

    const entries = fs.readdirSync(src, { withFileTypes: true })

    for (const entry of entries) {
        const srcPath = path.join(src, entry.name)
        const destPath = path.join(dest, entry.name)

        if (entry.isDirectory()) {
            copyDir(srcPath, destPath)
        } else {
            fs.copyFileSync(srcPath, destPath)
        }
    }
}
