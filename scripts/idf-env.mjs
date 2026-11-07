import { spawn } from 'node:child_process'
import { existsSync, readdirSync } from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const here = path.dirname(fileURLToPath(import.meta.url))

export const repoRoot = path.resolve(here, '..')
export const applicationsDir = path.join(repoRoot, 'applications')
export const idfActivate = 'D:\\sdk\\espressif\\Microsoft.v6.1.PowerShell_profile.ps1'
export const idfTarget = 'esp32p4'

export function applicationDir(name) {
  const dir = path.join(applicationsDir, name)
  if (!existsSync(path.join(dir, 'CMakeLists.txt'))) {
    throw new Error(`No ESP-IDF application at ${dir}`)
  }
  return dir
}

export function listApplications() {
  return readdirSync(applicationsDir, { withFileTypes: true })
    .filter((entry) => entry.isDirectory())
    .map((entry) => entry.name)
    .filter((name) => existsSync(path.join(applicationsDir, name, 'CMakeLists.txt')))
}

function psQuote(value) {
  return `'${String(value).replaceAll("'", "''")}'`
}

export function runIdf(appName, idfArgs) {
  const appDir = applicationDir(appName)
  const command = [
    `. ${psQuote(idfActivate)}`,
    `Set-Location -LiteralPath ${psQuote(appDir)}`,
    `idf.py ${idfArgs.map(psQuote).join(' ')}`,
    'exit $LASTEXITCODE',
  ].join('; ')

  return new Promise((resolve, reject) => {
    const child = spawn(
      'powershell.exe',
      ['-NoProfile', '-ExecutionPolicy', 'Bypass', '-Command', command],
      { stdio: 'inherit', windowsHide: true },
    )
    child.on('error', reject)
    child.on('exit', (code) => {
      if (code === 0) resolve()
      else reject(new Error(`idf.py ${idfArgs.join(' ')} failed in ${appName} (exit ${code})`))
    })
  })
}

export async function buildApplication(appName) {
  const appDir = applicationDir(appName)
  if (!existsSync(path.join(appDir, 'sdkconfig'))) {
    await runIdf(appName, ['set-target', idfTarget])
  }
  await runIdf(appName, ['build'])
}
