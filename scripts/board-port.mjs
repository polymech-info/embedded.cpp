import { SerialPort } from 'serialport'

export async function findBoardPort() {
  if (process.env.EMBEDDED_PORT) return process.env.EMBEDDED_PORT

  const ports = await SerialPort.list()
  const label = (port) => `${port.path} ${port.manufacturer || port.friendlyName || ''}`.trim()
  const espressif = ports.filter((port) => {
    const id = `${port.vendorId || ''}`.toLowerCase()
    return id === '303a' || /espressif|usb jtag/i.test(label(port))
  })

  if (espressif.length === 1) return espressif[0].path
  if (espressif.length > 1) {
    throw new Error(`Multiple Espressif ports (${espressif.map(label).join(', ')}). Set EMBEDDED_PORT.`)
  }
  if (ports.length === 1) return ports[0].path

  const seen = ports.map(label).join('\n')
  throw new Error(`No single board port found.\n${seen || '(no serial ports)'}\nSet EMBEDDED_PORT to override.`)
}
