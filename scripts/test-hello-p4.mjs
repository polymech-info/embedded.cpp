import { SerialPort } from 'serialport'
import { ReadlineParser } from '@serialport/parser-readline'
import { buildApplication, runIdf } from './idf-env.mjs'
import { findBoardPort } from './board-port.mjs'

const app = 'hello-p4'

function delay(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms))
}

function openPort(path) {
  const port = new SerialPort({ path, baudRate: 115200, autoOpen: false, hupcl: false })
  const parser = port.pipe(new ReadlineParser({ delimiter: '\n' }))
  const lines = []
  const waiters = []

  parser.on('data', (raw) => {
    const line = String(raw).replace(/\r/g, '').trimEnd()
    if (!line) return
    lines.push(line)
    if (line.startsWith('@')) console.log(line)
    for (const waiter of [...waiters]) {
      if (waiter.match(line)) {
        waiters.splice(waiters.indexOf(waiter), 1)
        waiter.resolve(line)
      }
    }
  })

  return new Promise((resolve, reject) => {
    port.open((error) => {
      if (error) reject(error)
      else resolve({ port, lines, waiters })
    })
  })
}

function waitFor(session, match, timeoutMs, label) {
  const existing = session.lines.find(match)
  if (existing) return Promise.resolve(existing)
  return new Promise((resolve, reject) => {
    const waiter = { match, resolve }
    const timer = setTimeout(() => {
      session.waiters.splice(session.waiters.indexOf(waiter), 1)
      const tail = session.lines.slice(-20).join('\n')
      reject(new Error(`Timed out waiting for ${label}\n${tail}`))
    }, timeoutMs)
    waiter.resolve = (line) => {
      clearTimeout(timer)
      resolve(line)
    }
    session.waiters.push(waiter)
  })
}

function send(port, command) {
  return new Promise((resolve, reject) => {
    port.write(`${command}\r\n`, (error) => (error ? reject(error) : port.drain((drainError) => (drainError ? reject(drainError) : resolve()))))
  })
}

function parseJson(line, tag) {
  const payload = line.slice(tag.length).trim()
  return JSON.parse(payload)
}

const portPath = await findBoardPort()
console.log(`board port ${portPath}`)
console.log('== build hello-p4')
await buildApplication(app)
console.log(`== flash hello-p4 on ${portPath}`)
await runIdf(app, ['-p', portPath, 'flash'])

await delay(1500)
const session = await openPort(portPath)
try {
  await waitFor(session, (line) => line === '@ready' || line === '@pong', 20000, '@ready').catch(() => {})
  await send(session.port, 'ping')
  const pong = await waitFor(session, (line) => line === '@pong', 8000, '@pong')
  if (pong !== '@pong') throw new Error(`ping expected @pong, got ${pong}`)

  await send(session.port, 'roundtrip')
  const summaryLine = await waitFor(session, (line) => line.startsWith('@roundtrip '), 60000, '@roundtrip')
  const summary = parseJson(summaryLine, '@roundtrip')
  console.log(`roundtrip wifi=${summary.wifi} audio=${summary.audio}`)
  if (summary.wifi !== 'ok' || summary.audio !== 'ok') {
    throw new Error(`roundtrip failed wifi=${summary.wifi} audio=${summary.audio}`)
  }
  console.log('test:hello-p4 ok')
} finally {
  await new Promise((resolve) => session.port.close(() => resolve()))
}
