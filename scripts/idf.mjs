import { buildApplication, listApplications, runIdf } from './idf-env.mjs'
import { findBoardPort } from './board-port.mjs'

const [action, appName] = process.argv.slice(2)

function usage() {
  console.error('Usage: node scripts/idf.mjs <build|flash|monitor> [application]')
  process.exit(2)
}

if (!action) usage()

const apps = appName ? [appName] : listApplications()
if (apps.length === 0) {
  console.error('No applications under applications/')
  process.exit(1)
}

if (action === 'build') {
  for (const app of apps) {
    console.log(`\n== build ${app}`)
    await buildApplication(app)
  }
} else if (action === 'flash' || action === 'monitor') {
  if (!appName) usage()
  const port = await findBoardPort()
  console.log(`${action} ${appName} on ${port}`)
  if (action === 'flash') await buildApplication(appName)
  await runIdf(appName, ['-p', port, action])
} else {
  usage()
}
